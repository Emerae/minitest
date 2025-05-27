
#include "../../includes/minishell.h"

/*
** HOOK D'ÉVÉNEMENTS READLINE POUR LES SIGNAUX
** Cette fonction est appelée périodiquement par readline pour vérifier
** si des signaux ont été reçus pendant la saisie.
** 
** Le problème : readline() bloque le programme en attendant l'input utilisateur.
** Pendant ce temps, si on reçoit un Ctrl-C, le signal est traité par handle_sigint()
** qui met g_signal_received = SIGINT, mais readline() continue d'attendre.
** 
** La solution : rl_event_hook est une fonction appelée régulièrement par readline.
** On peut l'utiliser pour vérifier g_signal_received et forcer readline à s'arrêter.
** 
** Actions quand Ctrl-C détecté :
** 1. rl_on_new_line() : dit à readline qu'on passe à la ligne suivante
** 2. rl_replace_line("", 0) : vide la ligne en cours de saisie
** 3. rl_done = 1 : force readline() à retourner immédiatement
** 
** Résultat : readline() retourne NULL, ce qui est intercepté dans shell_loop()
** pour afficher un nouveau prompt.
** 
** Retourne : 0 (valeur ignorée par readline)
*/
int	check_signals_hook(void)
{
	if (g_signal_received == SIGINT)
	{
		rl_on_new_line();
		rl_replace_line("", 0);
		rl_done = 1;
	}
	return (0);
}

/*
** GESTIONNAIRE DE SIGNAL SIGINT (Ctrl-C) EN MODE INTERACTIF
** Cette fonction est appelée automatiquement par le système quand l'utilisateur
** tape Ctrl-C pendant que minishell attend une commande.
** 
** Comportement attendu (comme bash) :
** - Ctrl-C pendant la saisie : annule la ligne en cours, nouveau prompt
** - Ctrl-C pendant l'exécution d'une commande : tue la commande, nouveau prompt
** 
** Ce qu'elle fait :
** 1. Stocke le numéro du signal dans g_signal_received (pour check_signals_hook)
** 2. Affiche un retour à la ligne (\n) pour que le prompt apparaisse proprement
** 
** Note : on n'affiche PAS de nouveau prompt ici ! C'est shell_loop() qui s'en charge
** quand il détecte que g_signal_received != 0.
** 
** Paramètres :
** - sig : numéro du signal reçu (SIGINT = 2)
*/
void	handle_sigint(int sig)
{
	g_signal_received = sig;
	write(STDOUT_FILENO, "\n", 1);
}

/*
** GESTIONNAIRE DE SIGNAL SIGQUIT (Ctrl-\) EN MODE INTERACTIF
** Cette fonction est appelée quand l'utilisateur tape Ctrl-\.
** 
** Comportement bash :
** - En mode interactif : Ctrl-\ ne fait rien (ignoré)
** - Pendant l'exécution d'une commande : Ctrl-\ tue la commande avec SIGQUIT
** 
** Notre implémentation : on ignore complètement Ctrl-\ en mode interactif.
** C'est pour ça que la fonction est vide - elle ne fait littéralement rien.
** 
** Pourquoi cette fonction existe-t-elle si elle ne fait rien ?
** Parce qu'on doit explicitement dire au système qu'on veut ignorer SIGQUIT.
** Si on n'installe pas de handler, le comportement par défaut est de quitter le programme.
** 
** Paramètres :
** - sig : numéro du signal reçu (SIGQUIT = 3)
*/
void	handle_sigquit(int sig)
{
	(void)sig;
}

/*
** CONFIGURATION DES SIGNAUX POUR LE PROCESSUS PARENT (MODE INTERACTIF)
** Cette fonction configure comment le shell principal réagit aux signaux
** quand il attend une commande de l'utilisateur.
** 
** Configuration :
** 1. SIGINT (Ctrl-C) : handle_sigint() - affiche nouveau prompt
** 2. SIGQUIT (Ctrl-\) : SIG_IGN - ignoré complètement
** 3. rl_event_hook : check_signals_hook - pour interrompre readline
** 
** Structures sigaction utilisées :
** - sa_handler : pointeur vers la fonction de gestion
** - sa_mask : signaux à bloquer pendant l'exécution du handler (aucun)
** - sa_flags : options spéciales (aucune)
** 
** Pourquoi sigaction() plutôt que signal() ?
** - Plus portable et plus fiable
** - Contrôle plus fin du comportement
** - Évite les comportements bizarres selon les systèmes
** 
** Appelée : au démarrage du shell et après chaque commande
*/
void	setup_signals(void)
{
	struct sigaction	sa_int;
	struct sigaction	sa_quit;

	g_signal_received = 0;
	rl_event_hook = check_signals_hook;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_handler = handle_sigint;
	sa_int.sa_flags = 0;
	sigaction(SIGINT, &sa_int, NULL);
	sigemptyset(&sa_quit.sa_mask);
	sa_quit.sa_handler = SIG_IGN;
	sa_quit.sa_flags = 0;
	sigaction(SIGQUIT, &sa_quit, NULL);
}

/*
** CONFIGURATION DES SIGNAUX POUR LES PROCESSUS ENFANTS (COMMANDES)
** Cette fonction remet les signaux à leur comportement par défaut
** pour les processus enfants qui exécutent des commandes.
** 
** Pourquoi faire ça ?
** Les commandes externes (ls, grep, cat, etc.) s'attendent au comportement
** standard des signaux Unix :
** - Ctrl-C (SIGINT) : tue le processus (exit code 130)
** - Ctrl-\ (SIGQUIT) : tue le processus + core dump (exit code 131)
** 
** Si on gardait nos handlers personnalisés, les commandes ne réagiraient
** pas correctement aux signaux.
** 
** SIG_DFL = comportement par défaut du système pour chaque signal.
** 
** Exemple concret :
** 1. Utilisateur tape "sleep 10"
** 2. On fork() pour créer un processus enfant
** 3. Dans l'enfant : setup_child_signals() remet les signaux par défaut
** 4. execve("sleep") remplace le processus par le vrai programme sleep
** 5. Utilisateur tape Ctrl-C : sleep reçoit SIGINT et se termine normalement
** 
** Appelée : dans chaque processus enfant, juste avant execve()
*/
void	setup_child_signals(void)
{
	struct sigaction	sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
}

/*
** GESTIONNAIRE SPÉCIAL POUR SIGINT PENDANT LES HEREDOCS
** Cette fonction gère Ctrl-C spécifiquement pendant la saisie des heredocs (<<).
** 
** Problème spécifique aux heredocs :
** Quand on fait "cat << EOF", on entre en mode saisie spécial où readline()
** affiche "> " et attend les lignes jusqu'à "EOF".
** 
** Si l'utilisateur tape Ctrl-C pendant ce mode :
** 1. On veut annuler complètement le heredoc
** 2. On veut revenir au prompt normal
** 3. On ne veut PAS exécuter la commande cat
** 
** Actions de ce handler :
** 1. Marque qu'on a reçu SIGINT (pour que preprocess_heredocs() le détecte)
** 2. Ferme STDIN pour forcer readline() à retourner NULL
** 
** Résultat : readline() dans preprocess_heredocs() retourne NULL,
** ce qui est interprété comme "annuler le heredoc".
** 
** Paramètres :
** - sig : numéro du signal reçu (SIGINT = 2)
*/
void	handle_sigint_heredoc(int sig)
{
	g_signal_received = sig;
	close(STDIN_FILENO);
}

/*
** CONFIGURATION DES SIGNAUX POUR LE MODE HEREDOC
** Cette fonction installe le handler spécial pour les heredocs.
** 
** Utilisée seulement pendant preprocess_heredocs() quand on lit
** l'input utilisateur pour un heredoc.
** 
** Différence avec setup_signals() :
** - SIGINT : handle_sigint_heredoc() au lieu de handle_sigint()
** - Pas de rl_event_hook (on utilise pas readline() normalement)
** - Comportement plus agressif (ferme STDIN)
** 
** Séquence typique :
** 1. setup_heredoc_signals() - installe le handler spécial
** 2. Boucle readline() pour lire les lignes du heredoc
** 3. Si Ctrl-C : handle_sigint_heredoc() ferme STDIN
** 4. readline() retourne NULL, on sort de la boucle
** 5. setup_signals() - remet les handlers normaux
** 
** Appelée : uniquement dans preprocess_heredocs()
*/
void	setup_heredoc_signals(void)
{
	struct sigaction	sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handle_sigint_heredoc;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}

/*
#include "../../includes/minishell.h"

int	check_signals_hook(void)
{
	if (g_signal_received == SIGINT)
	{
		rl_on_new_line();
		rl_replace_line("", 0);
		rl_done = 1;
	}
	return (0);
}

void	handle_sigint(int sig)
{
	g_signal_received = sig;
	write(STDOUT_FILENO, "\n", 1);
}

void	handle_sigquit(int sig)
{
	(void)sig;
}

void	setup_signals(void)
{
	struct sigaction	sa_int;
	struct sigaction	sa_quit;

	g_signal_received = 0;
	rl_event_hook = check_signals_hook;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_handler = handle_sigint;
	sa_int.sa_flags = 0;
	sigaction(SIGINT, &sa_int, NULL);
	sigemptyset(&sa_quit.sa_mask);
	sa_quit.sa_handler = SIG_IGN;
	sa_quit.sa_flags = 0;
	sigaction(SIGQUIT, &sa_quit, NULL);
}

void	setup_child_signals(void)
{
	struct sigaction	sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
}

void	handle_sigint_heredoc(int sig)
{
	g_signal_received = sig;
	//write(STDOUT_FILENO, "^C", 2); 
	close(STDIN_FILENO);
}

void	setup_heredoc_signals(void)
{
	struct sigaction	sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handle_sigint_heredoc;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}
*/
