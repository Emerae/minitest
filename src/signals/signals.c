#include "../../includes/minishell.h"


/*
** Event hook called periodically by readline
** Check for received signals
*/
int	check_signals_hook(void)
{
	if (g_signal_received == SIGINT)
	{
		//write(STDOUT_FILENO, "\n", 1);
		rl_on_new_line();
		rl_replace_line("", 0);
		rl_done = 1;  /* Tell readline to return */
	}
	return (0);
}

/*
** Signal handler for SIGINT (Ctrl-C)
** In interactive mode: displays a new prompt on a new line
*/
void	handle_sigint(int sig)
{
	g_signal_received = sig;
	
	/* Envoyer SIGINT à tout le process group (y compris l'enfant) */
	write(STDOUT_FILENO, "\n", 1);
}

/*
** Signal handler for SIGQUIT (Ctrl-\)
** Does nothing in interactive mode
*/
void	handle_sigquit(int sig)
{
	(void)sig;
}


void	setup_signals(void)
{
	struct sigaction	sa_int;
	struct sigaction	sa_quit;

	g_signal_received = 0;
	rl_event_hook = check_signals_hook;  /* Set the hook */
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
** Setup signal handlers for parent process (interactive mode)
** SIGINT: custom handler that displays new prompt
** SIGQUIT: ignored

void	setup_signals(void)
{
	struct sigaction	sa_int;
	struct sigaction	sa_quit;

	g_signal_received = 0;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_handler = handle_sigint;
	sa_int.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa_int, NULL);
	sigemptyset(&sa_quit.sa_mask);
	sa_quit.sa_handler = SIG_IGN;
	sa_quit.sa_flags = 0;
	sigaction(SIGQUIT, &sa_quit, NULL);
}
*/

/*
** Setup signal handlers for child process
** Reset to default behavior for proper command execution
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
** Signal handler for heredoc mode
** Needs special handling to exit heredoc on Ctrl-C
*/
void	handle_sigint_heredoc(int sig)
{
	g_signal_received = sig;
	//write(STDOUT_FILENO, "^C", 2); 
	close(STDIN_FILENO);
}

/*
** Setup signals for heredoc mode
*/
void	setup_heredoc_signals(void)
{
	struct sigaction	sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handle_sigint_heredoc;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}
