#include "../../includes/minishell.h"

/*
** VÉRIFICATION SI UNE COMMANDE EST UN BUILTIN
** Cette fonction vérifie si la commande donnée est une de nos commandes intégrées.
** 
** Les builtins sont des commandes codées directement dans notre shell au lieu
** d'appeler des programmes externes. On a 7 builtins : echo, cd, pwd, export, unset, env, exit.
** 
** Pourquoi des builtins ?
** - cd : doit changer le répertoire du shell parent, pas d'un processus enfant
** - export/unset : doivent modifier l'environnement du shell parent
** - exit : doit quitter le shell parent
** - echo/pwd/env : optimisation (pas besoin de fork/execve)
** 
** Paramètres :
** - cmd : nom de la commande à vérifier (ex: "echo", "ls", etc.)
** 
** Retourne : 1 si c'est un builtin, 0 sinon
*/
int	is_builtin(char *cmd)
{
	if (!cmd)
		return (0);
	if (ft_strcmp(cmd, "echo") == 0)
		return (1);
	if (ft_strcmp(cmd, "cd") == 0)
		return (1);
	if (ft_strcmp(cmd, "pwd") == 0)
		return (1);
	if (ft_strcmp(cmd, "export") == 0)
		return (1);
	if (ft_strcmp(cmd, "unset") == 0)
		return (1);
	if (ft_strcmp(cmd, "env") == 0)
		return (1);
	if (ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

/*
** EXÉCUTION D'UNE COMMANDE BUILTIN
** Cette fonction dispatche l'exécution vers la bonne fonction builtin.
** C'est un simple switch/case sous forme de if/else (switch interdit par la norme).
** 
** Architecture : chaque builtin a sa propre fonction (builtin_echo, builtin_cd, etc.)
** qui gère les arguments et la logique spécifique.
** 
** Paramètres :
** - cmd : structure contenant la commande et ses arguments
** - shell : contexte du shell (environnement, codes de sortie, etc.)
** 
** Retourne : code de sortie de la commande (0 = succès, autre = erreur)
*/
int	execute_builtin(t_cmd *cmd, t_shell *shell)
{
	if (ft_strcmp(cmd->args[0], "echo") == 0)
		return (builtin_echo(cmd->args));
	if (ft_strcmp(cmd->args[0], "cd") == 0)
		return (builtin_cd(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "pwd") == 0)
		return (builtin_pwd());
	if (ft_strcmp(cmd->args[0], "export") == 0)
		return (builtin_export(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "unset") == 0)
		return (builtin_unset(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "env") == 0)
		return (builtin_env(shell->env));
	if (ft_strcmp(cmd->args[0], "exit") == 0)
		return (builtin_exit(cmd->args, shell));
	return (0);
}

/*
** EXÉCUTION DANS LE PROCESSUS ENFANT
** Cette fonction s'exécute après le fork(), dans le processus enfant.
** Elle ne retourne JAMAIS - elle finit toujours par exit().
** 
** Étapes :
** 1. Configure les signaux pour le processus enfant (Ctrl-C tue la commande)
** 2. Met en place les redirections (>, <, >>, <<)
** 3. Si c'est un builtin, l'exécute et exit avec son code de retour
** 4. Sinon, cherche l'exécutable dans le PATH
** 5. Lance execve() qui remplace le processus par la nouvelle commande
** 6. Si execve() échoue, affiche l'erreur et exit
** 
** Paramètres :
** - cmd : commande à exécuter avec ses arguments et redirections
** - shell : contexte du shell (surtout pour l'environnement)
*/
static void	execute_child_process(t_cmd *cmd, t_shell *shell)
{
	char	*cmd_path;

	setup_child_signals();
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
	free(cmd_path);
	exit(ERROR_PERMISSION);
}

/*
** ATTENTE DU PROCESSUS ENFANT ET RÉCUPÉRATION DU CODE DE SORTIE
** Cette fonction attend qu'un processus enfant se termine et récupère son code de sortie.
** 
** Gestion des codes de sortie :
** - Si le processus s'est terminé normalement : retourne son code de sortie (0-255)
** - Si le processus a été tué par un signal : retourne 128 + numéro du signal
**   Exemple : Ctrl-C (SIGINT = 2) donne 128 + 2 = 130 (comme bash)
** 
** Paramètres :
** - pid : PID du processus enfant à attendre
** 
** Retourne : code de sortie formaté selon les conventions bash
*/
static int	wait_child_process(pid_t pid)
{
	int	status;

	status = 0;
	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (WEXITSTATUS(status));
}

/*
** EXÉCUTION D'UNE COMMANDE EXTERNE AVEC FORK/EXECVE
** Cette fonction lance une commande externe (ls, grep, cat, etc.) dans un processus séparé.
** 
** Processus :
** 1. fork() crée un processus enfant identique au parent
** 2. Dans l'enfant (pid == 0) : on exécute execute_child_process() qui fait execve()
** 3. Dans le parent (pid > 0) : on attend que l'enfant se termine avec waitpid()
** 4. On récupère le code de sortie et on le retourne
** 
** Gestion des signaux : on stocke le PID enfant dans shell->current_child_pid
** pour pouvoir le tuer si on reçoit un Ctrl-C.
** 
** Paramètres :
** - cmd : commande à exécuter
** - shell : contexte du shell
** 
** Retourne : code de sortie de la commande exécutée
*/
static int	execute_external_command(t_cmd *cmd, t_shell *shell)
{
	pid_t	pid;
	int		result;

	pid = fork();
	if (pid == -1)
	{
		perror("minishell: fork");
		return (1);
	}
	if (pid == 0)
		execute_child_process(cmd, shell);
	shell->current_child_pid = pid;
	result = wait_child_process(pid);
	shell->current_child_pid = 0;
	return (result);
}

/*
** VÉRIFICATION SI UN BUILTIN DOIT S'EXÉCUTER DANS LE PARENT
** Certains builtins DOIVENT s'exécuter dans le processus parent, pas dans un enfant.
** 
** Pourquoi ?
** - cd : doit changer le répertoire du shell parent (sinon ça sert à rien)
** - export/unset : doivent modifier l'environnement du shell parent
** - exit : doit quitter le shell parent, pas juste un processus enfant
** 
** Les autres builtins (echo, pwd, env) peuvent s'exécuter dans un enfant sans problème.
** 
** Cas d'usage : quand on a des redirections avec un builtin.
** Exemple : "export VAR=value > file" - on veut que export modifie l'env parent
** mais qu'il écrive dans le fichier.
** 
** Paramètres :
** - cmd : nom de la commande builtin
** 
** Retourne : 1 si doit s'exécuter dans le parent, 0 sinon
*/
static int	must_run_in_parent(char *cmd)
{
	if (ft_strcmp(cmd, "export") == 0)
		return (1);
	if (ft_strcmp(cmd, "unset") == 0)
		return (1);
	if (ft_strcmp(cmd, "cd") == 0)
		return (1);
	if (ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

/*
** EXÉCUTION D'UN BUILTIN AVEC REDIRECTIONS DANS LE PARENT
** Cette fonction permet d'exécuter un builtin dans le processus parent tout
** en gérant les redirections (>, <, etc.).
** 
** Problème à résoudre : comment faire "export VAR=value > file" ?
** - export doit modifier l'environnement du parent
** - mais la sortie doit aller dans le fichier
** 
** Solution :
** 1. Sauvegarder les descripteurs stdin/stdout actuels
** 2. Appliquer les redirections (dup2 vers les fichiers)
** 3. Exécuter le builtin (qui écrit maintenant dans les bons fichiers)
** 4. Restaurer les descripteurs originaux
** 
** Paramètres :
** - cmd : commande builtin avec ses redirections
** - shell : contexte du shell
** 
** Retourne : code de sortie du builtin
*/
static int	execute_builtin_with_redirs(t_cmd *cmd, t_shell *shell)
{
	int	saved_stdout;
	int	saved_stdin;
	int	ret;

	saved_stdout = dup(STDOUT_FILENO);
	saved_stdin = dup(STDIN_FILENO);
	if (saved_stdout == -1 || saved_stdin == -1)
		return (1);
	if (setup_redirections(cmd->redirs) == -1)
	{
		close(saved_stdout);
		close(saved_stdin);
		return (1);
	}
	ret = execute_builtin(cmd, shell);
	dup2(saved_stdout, STDOUT_FILENO);
	dup2(saved_stdin, STDIN_FILENO);
	close(saved_stdout);
	close(saved_stdin);
	return (ret);
}

/*
** EXÉCUTION D'UNE COMMANDE SIMPLE (SANS PIPES)
** Cette fonction gère l'exécution d'une seule commande, qu'elle soit builtin ou externe.
** "Simple" = pas de pipes, juste une commande avec des arguments et éventuellement des redirections.
** 
** Logique de décision :
** 1. Si c'est pas une commande valide -> retourne 0
** 2. Si c'est un builtin :
**    a. Si il y a des redirections ET que le builtin doit tourner dans le parent 
**       -> execute_builtin_with_redirs()
**    b. Si il y a des redirections mais le builtin peut tourner dans un enfant
**       -> execute_external_command() (fork + execve)  
**    c. Sinon -> execute_builtin() directement
** 3. Si c'est une commande externe -> execute_external_command()
** 
** Exemples :
** - "echo hello" -> execute_builtin() direct
** - "echo hello > file" -> execute_external_command() (fork)
** - "export VAR=val > file" -> execute_builtin_with_redirs() (parent)
** - "ls -la" -> execute_external_command() (fork + execve)
** 
** Paramètres :
** - cmd : commande à exécuter
** - shell : contexte du shell
** 
** Retourne : code de sortie de la commande
*/
int	execute_simple_command(t_cmd *cmd, t_shell *shell)
{
	if (!cmd || !cmd->args || !cmd->args[0])
		return (0);
	if (is_builtin(cmd->args[0]))
	{
		if (cmd->redirs && must_run_in_parent(cmd->args[0]))
			return (execute_builtin_with_redirs(cmd, shell));
		if (cmd->redirs)
			return (execute_external_command(cmd, shell));
		return (execute_builtin(cmd, shell));
	}
	return (execute_external_command(cmd, shell));
}

/*
** POINT D'ENTRÉE PRINCIPAL DE L'EXÉCUTION
** Cette fonction décide comment exécuter une ligne de commande selon sa complexité.
** 
** Types de commandes :
** 1. Commande simple : "echo hello", "ls -la > file"
**    -> execute_simple_command()
** 
** 2. Pipeline : "ls | grep test", "cat file | sort | uniq"
**    -> execute_pipeline() (gestion des pipes entre commandes)
** 
** 3. Ligne vide ou invalide : retourne 0
** 
** Comment on détecte un pipeline ?
** Si cmd_list->next existe, c'est qu'on a plusieurs commandes liées par des pipes.
** 
** Paramètres :
** - cmd_list : liste chaînée des commandes à exécuter
** - shell : contexte du shell
** 
** Retourne : code de sortie de la dernière commande exécutée
*/
int	execute_command_line(t_cmd *cmd_list, t_shell *shell)
{
	if (!cmd_list)
		return (0);
	if (!cmd_list->next)
		return (execute_simple_command(cmd_list, shell));
	return (execute_pipeline(cmd_list, shell));
}

/*
#include "../../includes/minishell.h"


int	is_builtin(char *cmd)
{
	if (!cmd)
		return (0);
	if (ft_strcmp(cmd, "echo") == 0)
		return (1);
	if (ft_strcmp(cmd, "cd") == 0)
		return (1);
	if (ft_strcmp(cmd, "pwd") == 0)
		return (1);
	if (ft_strcmp(cmd, "export") == 0)
		return (1);
	if (ft_strcmp(cmd, "unset") == 0)
		return (1);
	if (ft_strcmp(cmd, "env") == 0)
		return (1);
	if (ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

int	execute_builtin(t_cmd *cmd, t_shell *shell)
{
	if (ft_strcmp(cmd->args[0], "echo") == 0)
		return (builtin_echo(cmd->args));
	if (ft_strcmp(cmd->args[0], "cd") == 0)
		return (builtin_cd(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "pwd") == 0)
		return (builtin_pwd());
	if (ft_strcmp(cmd->args[0], "export") == 0)
		return (builtin_export(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "unset") == 0)
		return (builtin_unset(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "env") == 0)
		return (builtin_env(shell->env));
	if (ft_strcmp(cmd->args[0], "exit") == 0)
		return (builtin_exit(cmd->args, shell));
	return (0);
}

static void	execute_child_process(t_cmd *cmd, t_shell *shell)
{
	char	*cmd_path;

	setup_child_signals();
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
	free(cmd_path);
	exit(ERROR_PERMISSION);
}

static int	wait_child_process(pid_t pid)
{
	int	status = 0;

	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (WEXITSTATUS(status));
}

static int	execute_external_command(t_cmd *cmd, t_shell *shell)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
	{
		perror("minishell: fork");
		return (1);
	}
	if (pid == 0)
		execute_child_process(cmd, shell);
	
	shell->current_child_pid = pid;
	int result = wait_child_process(pid);
	shell->current_child_pid = 0;
	
	return (result);
}

static int	must_run_in_parent(char *cmd)
{
	if (ft_strcmp(cmd, "export") == 0)
		return (1);
	if (ft_strcmp(cmd, "unset") == 0)
		return (1);
	if (ft_strcmp(cmd, "cd") == 0)
		return (1);
	if (ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

static int	execute_builtin_with_redirs(t_cmd *cmd, t_shell *shell)
{
	int	saved_stdout;
	int	saved_stdin;
	int	ret;

	saved_stdout = dup(STDOUT_FILENO);
	saved_stdin = dup(STDIN_FILENO);
	if (saved_stdout == -1 || saved_stdin == -1)
		return (1);
	if (setup_redirections(cmd->redirs) == -1)
	{
		close(saved_stdout);
		close(saved_stdin);
		return (1);
	}
	ret = execute_builtin(cmd, shell);
	dup2(saved_stdout, STDOUT_FILENO);
	dup2(saved_stdin, STDIN_FILENO);
	close(saved_stdout);
	close(saved_stdin);
	return (ret);
}

int	execute_simple_command(t_cmd *cmd, t_shell *shell)
{
	if (!cmd || !cmd->args || !cmd->args[0])
		return (0);
	if (is_builtin(cmd->args[0]))
	{
		if (cmd->redirs && must_run_in_parent(cmd->args[0]))
			return (execute_builtin_with_redirs(cmd, shell));
		if (cmd->redirs)
			return (execute_external_command(cmd, shell));
		return (execute_builtin(cmd, shell));
	}
	return (execute_external_command(cmd, shell));
}

int	execute_command_line(t_cmd *cmd_list, t_shell *shell)
{
	if (!cmd_list)
		return (0);
	if (!cmd_list->next)
		return (execute_simple_command(cmd_list, shell));
	return (execute_pipeline(cmd_list, shell));
}

*/