
#include "../includes/minishell.h"

/*
** Variable globale obligatoire pour les signaux (Ctrl-C, etc.)
** La norme 42 autorise qu'UNE SEULE variable globale, et c'est pour ça qu'on l'utilise.
** volatile = dit au compilateur de pas optimiser cette variable car elle peut changer à tout moment
** sig_atomic_t = type spécial pour les signaux, garanti d'être atomique (pas de corruption)
*/
volatile sig_atomic_t	g_signal_received = 0;

/*
** INITIALISATION DU SHELL
** Cette fonction initialise notre structure shell au démarrage du programme.
** C'est comme préparer ton bureau avant de bosser : on met tout en place.
** 
** Paramètres :
** - shell : notre structure principale qui contient tout l'état du shell
** - envp : l'environnement système (PATH, HOME, etc.) qu'on récupère de main()
**
** Ce qu'elle fait :
** 1. Copie l'environnement système dans notre structure (on va peut-être le modifier avec export/unset)
** 2. Initialise le code de sortie de la dernière commande à 0 (pour $?)
** 3. Met tous les flags à 0 (in_pipe, in_child, etc.)
** 4. Si la copie de l'environnement foire, on quitte direct (pas la peine de continuer)
*/
void	init_shell(t_shell *shell, char **envp)
{
	shell->env = copy_env(envp);
	if (!shell->env)
	{
		write(STDERR_FILENO, "minishell: failed to copy environment\n", 38);
		exit(1);
	}
	shell->last_exit_status = 0;
	shell->in_pipe = 0;
	shell->in_child = 0;
	shell->current_child_pid = 0;
}

/*
** NETTOYAGE DU SHELL
** Fonction de nettoyage : on libère la mémoire avant de quitter.
** Toujours nettoyer derrière soi, sinon valgrind va gueuler !
**
** Cette fonction est appelée juste avant exit() pour éviter les leaks mémoire.
** Elle libère tout le tableau d'environnement qu'on avait copié au début.
*/
void	cleanup_shell(t_shell *shell)
{
	if (shell->env)
		free_env(shell->env);
}

/*
** ÉTAPE 1 : PARSING ET VALIDATION DE BASE
** Cette fonction prend la ligne tapée par l'utilisateur et la transforme en tokens.
** Exemple : "ls -la | grep test" devient une liste de tokens [ls] [-la] [|] [grep] [test]
**
** Ce qu'elle fait en détail :
** 1. Appelle cy1_make_list() du parser pour découper la ligne en tokens
**    Le parser gère les guillemets, espaces, caractères spéciaux, etc.
** 2. Vérifie si c'est pas juste des espaces (type == 1 et un seul token)
** 3. Prépare l'expansion des variables avec cy3_substi_check()
**    Ça ajoute des métadonnées aux tokens pour savoir quoi expand plus tard
**
** Retourne :
** - head_input : liste de tokens prête pour l'étape suivante
** - NULL : si erreur ou ligne vide (dans ce cas on ignore la ligne)
*/
static t_input	*parse_and_validate_input(char *line, t_shell *shell)
{
	t_input	*head_input;

	head_input = cy1_make_list(line);
	if (!head_input)
		return (NULL);
	if (head_input->type == 1 && !head_input->next)
	{
		cy0_free_input_list(head_input);
		return (NULL);
	}
	if (cy3_substi_check(&head_input, shell->env))
	{
		cy0_free_input_list(head_input);
		shell->last_exit_status = 1;
		return (NULL);
	}
	return (head_input);
}

/*
** ÉTAPE 2 : EXPANSION DES VARIABLES ET VÉRIFICATION SYNTAXIQUE
** C'est ici qu'on remplace $HOME par /home/ton_user, $? par le code de sortie, etc.
** Puis on vérifie que la syntaxe est correcte (pas de "| |" ou "> >" par exemple).
**
** Détail des opérations :
** 1. cy3_scan_dollar_syntax() remplace toutes les variables par leur valeur
**    Exemple : "echo $HOME" devient "echo /home/user"
**    Le 3ème paramètre = valeur de $? (exit status de la dernière commande)
** 2. cy1_remove_space_nodes() enlève tous les tokens "espace" de la liste
**    Maintenant qu'on a fait l'expansion, les espaces servent plus à rien
** 3. 5 fonctions de validation syntaxique :
**    - cy4_1wrong_char : caractères interdits (\, *, etc.)
**    - cy4_2wrong_redir : redirections malformées (>>>, ><, etc.)
**    - cy4_3wrong_pipe : pipes malformés
**    - cy4_4wrong_redir_log : logique des redirections (> sans fichier, etc.)
**    - cy4_5wrong_pipe_log : logique des pipes (| en début/fin, etc.)
**
** Retourne : 0 si tout va bien, 1 si erreur de syntaxe
*/
static int	expand_and_check_syntax(t_input *head_input, t_shell *shell)
{
	if (cy3_scan_dollar_syntax(head_input, shell->env,
			shell->last_exit_status))
	{
		shell->last_exit_status = 1;
		return (1);
	}
	cy1_remove_space_nodes(&head_input);
	if (cy4_1wrong_char(head_input) || cy4_2wrong_redir(head_input)
		|| cy4_3wrong_pipe(head_input) || cy4_4wrong_redir_log(head_input)
		|| cy4_5wrong_pipe_log(head_input))
	{
		write(STDERR_FILENO, "minishell: syntax error\n", 24);
		shell->last_exit_status = ERROR_SYNTAX;
		return (1);
	}
	return (0);
}

/*
** ÉTAPE 3 : CONVERSION EN COMMANDES ET EXÉCUTION
** On transforme nos tokens en vraies commandes avec arguments et redirections, puis on les exécute.
**
** Ce qui se passe :
** 1. cy2_convert_cmd() transforme la liste de tokens en liste de commandes
**    Exemple : [ls] [-la] [|] [grep] [test] devient 2 commandes :
**    cmd1: args=["ls", "-la"], redirs=NULL, next=cmd2
**    cmd2: args=["grep", "test"], redirs=NULL, next=NULL
**
** 2. execute_command_line() lance l'exécution des commandes
**    Cette fonction gère :
**    - Les builtins (echo, cd, pwd, etc.) vs commandes externes (ls, grep, etc.)
**    - Les pipes entre les commandes
**    - Les redirections (>, <, >>, <<)
**    - Les processus enfants avec fork/execve
**
** 3. Nettoyage de la structure de commandes
**
** Retourne : le code de sortie de la dernière commande exécutée
*/
static int	convert_and_execute(t_input *head_input, t_shell *shell)
{
	t_cmd	*head_cmd;
	int		exit_status;

	head_cmd = cy2_convert_cmd(head_input);
	if (!head_cmd)
		return (1);
	exit_status = execute_command_line(head_cmd, shell);
	cy0_free_cmd_list(head_cmd);
	return (exit_status);
}

/*
** ORCHESTRATEUR PRINCIPAL
** Cette fonction coordonne tout le processus de traitement d'une ligne de commande.
** C'est le chef d'orchestre qui appelle les 3 étapes dans l'ordre.
**
** Flow :
** 1. ÉTAPE 1 : Parser et validation de base
** 2. ÉTAPE 2 : Expansion des variables et vérification syntaxique  
** 3. ÉTAPE 3 : Conversion en commandes et exécution
**
** À chaque étape, si il y a une erreur, on nettoie la mémoire et on s'arrête.
** Le code de sortie est automatiquement mis à jour dans shell->last_exit_status.
*/
static void	process_line(char *line, t_shell *shell)
{
	t_input	*head_input;

	head_input = parse_and_validate_input(line, shell);
	if (!head_input)
		return ;
	if (expand_and_check_syntax(head_input, shell))
	{
		cy0_free_input_list(head_input);
		return ;
	}
	shell->last_exit_status = convert_and_execute(head_input, shell);
	cy0_free_input_list(head_input);
}

/*
** GESTION DE READLINE ET DES SIGNAUX
** readline() peut être interrompue par un Ctrl-C. Dans ce cas, on doit gérer ça proprement.
**
** Ce qu'elle fait :
** 1. Appelle readline() qui affiche le prompt et attend que l'utilisateur tape quelque chose
** 2. Si on reçoit un Ctrl-C pendant readline :
**    - Met à jour le code de sortie à 130 (code bash pour Ctrl-C)
**    - Reset le flag de signal
**    - Tue le processus enfant s'il y en a un en cours
**    - Libère la mémoire si readline avait réussi à lire quelque chose
**    - Retourne NULL pour dire "ignore cette ligne"
**
** Retourne :
** - La ligne tapée par l'utilisateur (à libérer avec free())
** - NULL si Ctrl-C ou autre interruption
*/
static char	*handle_readline_and_signals(t_shell *shell)
{
	char	*line;

	line = readline(PROMPT);
	if (g_signal_received == SIGINT)
	{
		shell->last_exit_status = 130;
		g_signal_received = 0;
		if (shell->current_child_pid > 0)
		{
			kill(shell->current_child_pid, SIGINT);
			shell->current_child_pid = 0;
		}
		if (line)
			free(line);
		return (NULL);
	}
	return (line);
}

/*
** TRAITEMENT D'UNE LIGNE AVEC HISTORIQUE ET SIGNAUX
** Cette fonction s'occupe de tout ce qui entoure le traitement principal d'une ligne.
**
** Responsabilités :
** 1. Ajouter la ligne à l'historique readline (pour les flèches haut/bas)
** 2. Désactiver temporairement le hook des signaux pendant l'exécution
**    (sinon readline pourrait interférer avec nos commandes)
** 3. Appeler le traitement principal de la ligne (process_line)
** 4. Remettre le hook pour les signaux en mode interactif
** 5. Gérer les signaux qui arrivent pendant l'exécution de la commande
**
** Note : Si on reçoit un Ctrl-C pendant l'exécution, le code de sortie est mis à 130.
*/
static void	process_input_line(char *line, t_shell *shell)
{
	add_history(line);
	rl_event_hook = NULL;
	g_signal_received = 0;
	process_line(line, shell);
	rl_event_hook = check_signals_hook;
	if (g_signal_received == SIGINT)
	{
		shell->last_exit_status = 130;
		g_signal_received = 0;
	}
}

/*
** BOUCLE PRINCIPALE DU SHELL
** C'est le cœur du programme : on boucle indéfiniment jusqu'à exit ou Ctrl-D.
**
** Le cycle de vie :
** 1. Afficher le prompt "minishell$ "
** 2. Lire une ligne (gestion des signaux incluse)
** 3. Si la ligne n'est pas vide, la traiter
** 4. Libérer la mémoire de la ligne
** 5. Recommencer
**
** Conditions de sortie :
** - Ctrl-D (EOF) : readline retourne NULL et g_signal_received == 0
** - Commande "exit" : appelée depuis execute_command_line()
** - Erreur fatale
**
** Gestion des signaux :
** - Ctrl-C : nouvelle ligne de prompt (continue la boucle)
** - Ctrl-D : sortie propre du shell
*/
void	shell_loop(t_shell *shell)
{
	char	*line;

	while (1)
	{
		line = handle_readline_and_signals(shell);
		if (!line)
		{
			if (g_signal_received == 0)
			{
				write(STDOUT_FILENO, "exit\n", 5);
				break ;
			}
			continue ;
		}
		if (*line)
			process_input_line(line, shell);
		free(line);
	}
}

/*
** FONCTION PRINCIPALE DU PROGRAMME
** Point d'entrée : elle initialise tout et lance la boucle principale.
**
** Séquence de démarrage :
** 1. Ignorer argc et argv (minishell ne prend pas d'arguments en ligne de commande)
** 2. Initialiser la structure shell (copie environnement, codes de sortie, etc.)
** 3. Configurer les gestionnaires de signaux (Ctrl-C, Ctrl-\, etc.)
** 4. Lancer la boucle principale du shell
** 5. Nettoyer la mémoire avant de quitter
** 6. Vider l'historique readline
** 7. Retourner le code de sortie de la dernière commande
**
** Le code de sortie retourné sera celui de la dernière commande exécutée,
** exactement comme bash fait.
*/
int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;

	(void)argc;
	(void)argv;
	init_shell(&shell, envp);
	setup_signals();
	shell_loop(&shell);
	cleanup_shell(&shell);
	rl_clear_history();
	return (shell.last_exit_status);
}

/*

#include "../includes/minishell.h"

volatile sig_atomic_t	g_signal_received = 0;


void	init_shell(t_shell *shell, char **envp)
{
	shell->env = copy_env(envp);
	if (!shell->env)
	{
		write(STDERR_FILENO, "minishell: failed to copy environment\n", 38);
		exit(1);
	}
	shell->last_exit_status = 0;
	shell->in_pipe = 0;
	shell->in_child = 0;
	shell->current_child_pid = 0;
}

void	cleanup_shell(t_shell *shell)
{
	if (shell->env)
		free_env(shell->env);
}

static void	process_line(char *line, t_shell *shell)
{
	t_input	*head_input;
	t_cmd	*head_cmd;
	
	head_input = cy1_make_list(line);
	if (!head_input)
		return ;
	if (head_input->type == 1 && !head_input->next)
	{
		cy0_free_input_list(head_input);
		return ;
	}
	if (cy3_substi_check(&head_input, shell->env))
	{
		cy0_free_input_list(head_input);
		shell->last_exit_status = 1;
		return ;
	}
	if (cy3_scan_dollar_syntax(head_input, shell->env, shell->last_exit_status))
	{
		cy0_free_input_list(head_input);
		shell->last_exit_status = 1;
		return ;
	}
	cy1_remove_space_nodes(&head_input);
	if (cy4_1wrong_char(head_input) || cy4_2wrong_redir(head_input)
		|| cy4_3wrong_pipe(head_input) || cy4_4wrong_redir_log(head_input)
		|| cy4_5wrong_pipe_log(head_input))
	{
		write(STDERR_FILENO, "minishell: syntax error\n", 24);
		cy0_free_input_list(head_input);
		shell->last_exit_status = ERROR_SYNTAX;
		return ;
	}
	head_cmd = cy2_convert_cmd(head_input);
	cy0_free_input_list(head_input);
	if (!head_cmd)
	{
		shell->last_exit_status = 1;
		return ;
	}
	shell->last_exit_status = execute_command_line(head_cmd, shell);
	cy0_free_cmd_list(head_cmd);
}

void	shell_loop(t_shell *shell)
{
	char	*line;

	while (1)
	{
		line = readline(PROMPT);
		
		if (g_signal_received == SIGINT)
		{
			shell->last_exit_status = 130;
			g_signal_received = 0;
			if (shell->current_child_pid > 0)
			{
				kill(shell->current_child_pid, SIGINT);
				shell->current_child_pid = 0;
			}
			if (line)
				free(line);
			continue;
		}
		if (!line)
		{
			write(STDOUT_FILENO, "exit\n", 5);
			break ;
		}
		if (*line)
		{
			add_history(line);
			rl_event_hook = NULL;
			g_signal_received = 0;
			process_line(line, shell);
			rl_event_hook = check_signals_hook;
			if (g_signal_received == SIGINT)
			{
				shell->last_exit_status = 130;
				g_signal_received = 0;
			}
		}
		free(line);
	}
}

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;

	(void)argc;
	(void)argv;
	init_shell(&shell, envp);
	setup_signals();
	shell_loop(&shell);
	cleanup_shell(&shell);
	rl_clear_history();
	return (shell.last_exit_status);
}

*/