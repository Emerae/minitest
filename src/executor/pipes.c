#include "../../includes/minishell.h"

/*
** CRÉATION D'UN PIPE AVEC GESTION D'ERREUR
** Cette fonction crée un pipe et gère les erreurs de création.
** 
** Un pipe c'est quoi ? Un "tuyau" qui connecte la sortie d'une commande
** à l'entrée de la suivante. Par exemple : "ls | grep test"
** - ls écrit dans pipe_fd[1] (bout écriture)  
** - grep lit depuis pipe_fd[0] (bout lecture)
** 
** pipe() du système crée 2 descripteurs de fichier :
** - pipe_fd[0] = bout lecture
** - pipe_fd[1] = bout écriture
** 
** Paramètres :
** - pipe_fd : tableau qui recevra les 2 descripteurs [lecture, écriture]
** 
** Retourne : 0 si succès, -1 si erreur
*/
int	create_pipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == -1)
	{
		perror("minishell: pipe");
		return (-1);
	}
	return (0);
}

/*
** CONFIGURATION DES PIPES POUR UN PROCESSUS ENFANT  
** Cette fonction configure les redirections de stdin/stdout d'un processus enfant
** pour qu'il soit connecté correctement dans le pipeline.
** 
** Schéma d'un pipeline "cmd1 | cmd2 | cmd3" :
** - cmd1 : stdin=terminal, stdout=pipe1[1] 
** - cmd2 : stdin=pipe1[0], stdout=pipe2[1]
** - cmd3 : stdin=pipe2[0], stdout=terminal
** 
** Paramètres :
** - pipe_fd : pipe actuel [lecture, écriture] (peut être NULL si dernière cmd)
** - prev_pipe : descripteur de lecture du pipe précédent (-1 si première cmd)
** - is_last : 1 si c'est la dernière commande du pipeline, 0 sinon
*/
void	setup_pipe_child(int pipe_fd[2], int prev_pipe, int is_last)
{
	if (prev_pipe != -1)
	{
		dup2(prev_pipe, STDIN_FILENO);
		close(prev_pipe);
	}
	if (!is_last)
	{
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
	}
}

/*
** FERMETURE DES PIPES DANS LE PROCESSUS PARENT
** Après chaque fork, le parent doit fermer les bouts de pipe qu'il n'utilise pas
** et sauvegarder le bout lecture pour la prochaine commande.
** 
** Logique :
** 1. Ferme le prev_pipe (on en a plus besoin, l'enfant l'a récupéré)
** 2. Ferme le bout écriture du pipe actuel (seul l'enfant écrit dedans)
** 3. Sauvegarde le bout lecture pour la commande suivante
** 
** Paramètres :
** - pipe_fd : pipe actuel (peut être NULL si dernière commande)
** - prev_pipe : pointeur vers le prev_pipe à mettre à jour
*/
void	close_pipe_parent(int pipe_fd[2], int *prev_pipe)
{
	if (*prev_pipe != -1)
		close(*prev_pipe);
	if (pipe_fd && pipe_fd[0] != -1)
	{
		close(pipe_fd[1]);
		*prev_pipe = pipe_fd[0];
	}
}

/*
** EXÉCUTION D'UNE COMMANDE DANS UN PIPELINE
** Cette fonction fork un processus enfant et configure tout pour qu'il
** s'exécute correctement dans le pipeline (pipes + redirections).
** 
** Séquence dans l'enfant :
** 1. Configure les signaux (Ctrl-C tue la commande)
** 2. Configure les pipes (entrée/sortie connectées aux bonnes commandes)
** 3. Applique les redirections spécifiques à cette commande (>, <, etc.)
** 4. Exécute la commande (builtin ou externe avec execve)
** 
** Paramètres :
** - cmd : commande à exécuter avec ses arguments et redirections
** - shell : contexte du shell (environnement, etc.)
** - prev_pipe : descripteur de lecture du pipe précédent (-1 si première)
** - pipe_fd : pipe actuel (NULL si dernière commande)
** 
** Retourne : PID du processus enfant créé, ou -1 si erreur
*/
static int	execute_piped_command(t_cmd *cmd, t_shell *shell,
								int prev_pipe, int pipe_fd[2])
{
	pid_t	pid;
	char	*cmd_path;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		setup_child_signals();
		shell->in_child = 1;
		setup_pipe_child(pipe_fd, prev_pipe, cmd->next == NULL);
		if (setup_redirections(cmd->redirs) == -1)
			exit(1);
		if (is_builtin(cmd->args[0]))
			exit(execute_builtin(cmd, shell));
		cmd_path = find_command_path(cmd->args[0], shell->env);
		if (!cmd_path)
		{
			print_error(cmd->args[0], "command not found");
			exit(ERROR_CMD_NOT_FOUND);
		}
		execve(cmd_path, cmd->args, shell->env);
		perror("minishell: execve");
		exit(ERROR_PERMISSION);
	}
	return (pid);
}

/*
** GÉNÉRATION D'UN NOM DE FICHIER TEMPORAIRE UNIQUE
** Cette fonction crée un nom de fichier unique pour les heredocs.
** On utilise un compteur statique pour éviter les collisions.
** 
** Format : "/tmp/minishell_heredoc_0", "/tmp/minishell_heredoc_1", etc.
** 
** Paramètres :
** - buffer : buffer où écrire le nom de fichier (doit être assez grand)
** - counter : compteur pour rendre le nom unique
*/
static void	create_temp_filename(char *buffer, int counter)
{
	char	*base;
	int		i;
	int		temp;

	base = "/tmp/minishell_heredoc_";
	i = 0;
	while (base[i])
	{
		buffer[i] = base[i];
		i++;
	}
	temp = counter;
	if (temp == 0)
		buffer[i++] = '0';
	else
	{
		if (temp >= 10)
			buffer[i++] = (temp / 10) + '0';
		buffer[i++] = (temp % 10) + '0';
	}
	buffer[i] = '\0';
}

/*
** TRAITEMENT D'UN SEUL HEREDOC
** Cette fonction gère un heredoc spécifique : lit l'input utilisateur
** ligne par ligne jusqu'au délimiteur et l'écrit dans un fichier temporaire.
** 
** Un heredoc (<<) fonctionne comme ça :
** cat << EOF
** ligne 1  
** ligne 2
** EOF
** -> "ligne 1\nligne 2\n" est passé à cat
** 
** Processus :
** 1. Crée un fichier temporaire unique
** 2. Lit les lignes utilisateur avec readline("> ")
** 3. Compare chaque ligne avec le délimiteur
** 4. Si c'est le délimiteur : stop
** 5. Sinon : écrit la ligne dans le fichier temporaire
** 6. Gère Ctrl-C : nettoie le fichier et retourne -1
** 
** Paramètres :
** - redir : structure de redirection contenant le délimiteur
** - temp_fd : descripteur du fichier temporaire ouvert
** - temp_filename : nom du fichier temporaire (pour le supprimer si Ctrl-C)
** 
** Retourne : 0 si succès, -1 si Ctrl-C ou erreur
*/
static int	process_single_heredoc(t_redir *redir, int temp_fd,
									char *temp_filename)
{
	char	*line;

	setup_heredoc_signals();
	while (1)
	{
		line = readline("> ");
		if (!line || g_signal_received == SIGINT)
		{
			if (g_signal_received == SIGINT)
			{
				g_signal_received = 0;
				close(temp_fd);
				unlink(temp_filename);
				setup_signals();
				return (-1);
			}
			break ;
		}
		if (ft_strcmp(line, redir->file) == 0)
		{
			free(line);
			break ;
		}
		write(temp_fd, line, ft_strlen(line));
		write(temp_fd, "\n", 1);
		free(line);
	}
	setup_signals();
	return (0);
}

/*
** PRÉPROCESSING DE TOUS LES HEREDOCS D'UN PIPELINE  
** Cette fonction traite tous les heredocs (<<) d'un pipeline AVANT l'exécution.
** 
** Pourquoi préprocesser ? Les heredocs bloquent le terminal pour lire l'input
** utilisateur. Si on fait ça pendant l'exécution du pipeline, ça devient le bordel
** avec les signaux et les processus enfants.
** 
** Stratégie :
** 1. Parcourt toutes les commandes du pipeline
** 2. Pour chaque heredoc trouvé :
**    a. Lit l'input utilisateur et l'écrit dans un fichier temporaire
**    b. Remplace la redirection heredoc par une redirection fichier normale
**    c. Le fichier temporaire sera lu comme un fichier normal pendant l'exécution
** 
** Exemple : "cat << EOF" devient "cat < /tmp/minishell_heredoc_0"
** 
** Gestion des signaux : si Ctrl-C pendant un heredoc, on annule tout le pipeline.
** 
** Paramètres :
** - cmd_list : liste des commandes du pipeline
** 
** Retourne : 0 si succès, -1 si Ctrl-C ou erreur
*/
static int	preprocess_heredocs(t_cmd *cmd_list)
{
	t_cmd			*current;
	t_redir			*redir;
	char			temp_filename[64];
	int				temp_fd;
	static int		heredoc_count = 0;

	current = cmd_list;
	while (current)
	{
		redir = current->redirs;
		while (redir)
		{
			if (redir->type == 3)
			{
				create_temp_filename(temp_filename, heredoc_count);
				heredoc_count++;
				temp_fd = open(temp_filename,
					O_WRONLY | O_CREAT | O_TRUNC, 0600);
				if (temp_fd == -1)
					return (-1);
				if (process_single_heredoc(redir, temp_fd, temp_filename) == -1)
					return (-1);
				close(temp_fd);
				free(redir->file);
				redir->file = ft_strdup(temp_filename);
				redir->type = 0;
			}
			redir = redir->next;
		}
		current = current->next;
	}
	return (0);
}

/*
** COMPTAGE DES COMMANDES DANS UN PIPELINE
** Cette fonction compte le nombre de commandes dans un pipeline pour
** allouer le bon nombre de PIDs.
** 
** Exemple : "ls | grep test | wc" -> 3 commandes
** 
** Paramètres :
** - cmd_list : première commande du pipeline
** 
** Retourne : nombre de commandes
*/
static int	count_pipeline_commands(t_cmd *cmd_list)
{
	t_cmd	*current;
	int		count;

	count = 0;
	current = cmd_list;
	while (current)
	{
		count++;
		current = current->next;
	}
	return (count);
}

/*
** EXÉCUTION DE TOUTES LES COMMANDES DU PIPELINE
** Cette fonction crée tous les processus enfants et configure les pipes.
** 
** Algorithme :
** 1. Pour chaque commande sauf la dernière :
**    a. Crée un pipe pour connecter à la commande suivante
**    b. Fork et exécute la commande avec les bons pipes
**    c. Ferme les bouts de pipe inutiles dans le parent
** 2. Pour la dernière commande :
**    a. Pas de pipe de sortie (stdout = terminal)
**    b. Fork et exécute avec seulement le pipe d'entrée
** 
** Paramètres :
** - cmd_list : première commande du pipeline
** - shell : contexte du shell
** - pids : tableau pour stocker les PIDs des processus créés
** - cmd_count : nombre total de commandes
** 
** Retourne : 0 si succès, -1 si erreur
*/
static int	execute_pipeline_commands(t_cmd *cmd_list, t_shell *shell,
										pid_t *pids, int cmd_count)
{
	t_exec	exec;
	t_cmd	*current;
	int		i;

	exec.prev_pipe = -1;
	current = cmd_list;
	i = 0;
	while (current && i < cmd_count)
	{
		if (current->next && create_pipe(exec.pipe_fd) == -1)
			return (-1);
		pids[i] = execute_piped_command(current, shell, exec.prev_pipe,
			current->next ? exec.pipe_fd : NULL);
		if (pids[i] == -1)
			return (-1);
		close_pipe_parent(current->next ? exec.pipe_fd : NULL,
			&exec.prev_pipe);
		current = current->next;
		i++;
	}
	return (0);
}

/*
** ATTENTE DE TOUS LES PROCESSUS DU PIPELINE
** Cette fonction attend que tous les processus enfants se terminent
** et récupère le code de sortie du dernier (comme bash).
** 
** Logique bash : dans un pipeline, seul le code de sortie de la dernière
** commande compte. "false | true" retourne 0, "true | false" retourne 1.
** 
** Paramètres :
** - pids : tableau des PIDs des processus à attendre
** - cmd_count : nombre de processus
** 
** Retourne : code de sortie de la dernière commande
*/
static int	wait_all_processes(pid_t *pids, int cmd_count)
{
	int	i;
	int	status;

	i = 0;
	status = 0;
	while (i < cmd_count)
	{
		waitpid(pids[i], &status, 0);
		i++;
	}
	if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (WEXITSTATUS(status));
}

/*
** EXÉCUTION COMPLÈTE D'UN PIPELINE
** Cette fonction orchestre l'exécution d'un pipeline complet (cmd1 | cmd2 | cmd3...).
** 
** Étapes principales :
** 1. Préprocessing des heredocs (lecture input utilisateur)
** 2. Comptage et allocation mémoire pour les PIDs
** 3. Exécution de toutes les commandes avec pipes
** 4. Attente de tous les processus
** 5. Nettoyage mémoire
** 
** Exemple pour "ls -la | grep test | wc -l" :
** 1. Pas de heredocs à traiter
** 2. 3 commandes -> alloue tableau de 3 PIDs
** 3. Crée 2 pipes et 3 processus enfants connectés
** 4. Attend les 3 processus
** 5. Retourne le code de sortie de wc
** 
** Gestion d'erreur : si une étape échoue, on nettoie et on retourne 1.
** 
** Paramètres :
** - cmd_list : première commande du pipeline
** - shell : contexte du shell
** 
** Retourne : code de sortie de la dernière commande du pipeline
*/
int	execute_pipeline(t_cmd *cmd_list, t_shell *shell)
{
	pid_t	*pids;
	int		cmd_count;
	int		result;

	if (preprocess_heredocs(cmd_list) == -1)
		return (1);
	cmd_count = count_pipeline_commands(cmd_list);
	pids = malloc(sizeof(pid_t) * cmd_count);
	if (!pids)
		return (1);
	if (execute_pipeline_commands(cmd_list, shell, pids, cmd_count) == -1)
	{
		free(pids);
		return (1);
	}
	result = wait_all_processes(pids, cmd_count);
	free(pids);
	return (result);
}


/*
#include "../../includes/minishell.h"

int	create_pipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == -1)
	{
		perror("minishell: pipe");
		return (-1);
	}
	return (0);
}

void	setup_pipe_child(int pipe_fd[2], int prev_pipe, int is_last)
{
	if (prev_pipe != -1)
	{
		dup2(prev_pipe, STDIN_FILENO);
		close(prev_pipe);
	}
	if (!is_last)
	{
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
	}
}

void	close_pipe_parent(int pipe_fd[2], int *prev_pipe)
{
	if (*prev_pipe != -1)
		close(*prev_pipe);
	if (pipe_fd && pipe_fd[0] != -1)
	{
		close(pipe_fd[1]);
		*prev_pipe = pipe_fd[0];
	}
}

static int	execute_piped_command(t_cmd *cmd, t_shell *shell,
								int prev_pipe, int pipe_fd[2])
{
	pid_t	pid;
	char	*cmd_path;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		setup_child_signals();
		shell->in_child = 1;
		setup_pipe_child(pipe_fd, prev_pipe, cmd->next == NULL);
		if (setup_redirections(cmd->redirs) == -1)
			exit(1);
		if (is_builtin(cmd->args[0]))
			exit(execute_builtin(cmd, shell));
		cmd_path = find_command_path(cmd->args[0], shell->env);
		if (!cmd_path)
		{
			print_error(cmd->args[0], "command not found");
			exit(ERROR_CMD_NOT_FOUND);
		}
		execve(cmd_path, cmd->args, shell->env);
		perror("minishell: execve");
		exit(ERROR_PERMISSION);
	}
	return (pid);
}

static void	create_temp_filename(char *buffer, int counter)
{
	char	*base;
	int		i;
	int		temp;

	base = "/tmp/minishell_heredoc_";
	i = 0;
	while (base[i])
	{
		buffer[i] = base[i];
		i++;
	}
	temp = counter;
	if (temp == 0)
		buffer[i++] = '0';
	else
	{
		if (temp >= 10)
			buffer[i++] = (temp / 10) + '0';
		buffer[i++] = (temp % 10) + '0';
	}
	buffer[i] = '\0';
}

static int	preprocess_heredocs(t_cmd *cmd_list)
{
	t_cmd		*current;
	t_redir		*redir;
	char		*line;
	char		temp_filename[64];
	int			temp_fd;
	static int	heredoc_count = 0;

	current = cmd_list;
	while (current)
	{
		redir = current->redirs;
		while (redir)
		{
			if (redir->type == 3)
			{
				create_temp_filename(temp_filename, heredoc_count);
				heredoc_count++;
				temp_fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
				if (temp_fd == -1)
					return (-1);
				setup_heredoc_signals();
				while (1)
				{
					line = readline("> ");
					if (!line || g_signal_received == SIGINT)
					{
						if (g_signal_received == SIGINT)
						{
							g_signal_received = 0;
							close(temp_fd);
							unlink(temp_filename);
							setup_signals();
							return (-1);
						}
						break ;
					}
					if (ft_strcmp(line, redir->file) == 0)
					{
						free(line);
						break ;
					}
					write(temp_fd, line, ft_strlen(line));
					write(temp_fd, "\n", 1);
					free(line);
				}
				close(temp_fd);
				setup_signals();
				free(redir->file);
				redir->file = ft_strdup(temp_filename);
				redir->type = 0;
			}
			redir = redir->next;
		}
		current = current->next;
	}
	return (0);
}

int	execute_pipeline(t_cmd *cmd_list, t_shell *shell)
{
	t_exec	exec;
	t_cmd	*current;
	pid_t	*pids;
	int		cmd_count;
	int		i;
	int		*pipe_for_cmd;
	int		*pipe_for_close;

	if (preprocess_heredocs(cmd_list) == -1)
		return (1);
	cmd_count = 0;
	current = cmd_list;
	while (current && ++cmd_count)
		current = current->next;
	pids = malloc(sizeof(pid_t) * cmd_count);
	if (!pids)
		return (1);
	exec.prev_pipe = -1;
	current = cmd_list;
	i = 0;
	while (current)
	{
		if (current->next && create_pipe(exec.pipe_fd) == -1)
		{
			free(pids);
			return (1);
		}
		if (current->next)
			pipe_for_cmd = exec.pipe_fd;
		else
			pipe_for_cmd = NULL;
		pids[i] = execute_piped_command(current, shell, exec.prev_pipe, pipe_for_cmd);
		if (pids[i] == -1)
		{
			free(pids);
			return (1);
		}
		if (current->next)
			pipe_for_close = exec.pipe_fd;
		else
			pipe_for_close = NULL;
		close_pipe_parent(pipe_for_close, &exec.prev_pipe);
		current = current->next;
		i++;
	}
	i = 0;
	exec.status = 0;
	while (i < cmd_count)
	{
		waitpid(pids[i], &exec.status, 0);
		i++;
	}
	free(pids);
	if (WIFSIGNALED(exec.status))
		return (128 + WTERMSIG(exec.status));
	return (WEXITSTATUS(exec.status));
}
*/
