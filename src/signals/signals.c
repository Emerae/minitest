#include "../../includes/minishell.h"

volatile sig_atomic_t	g_signal_received = 0;

/*
** Signal handler for SIGINT (Ctrl-C)
** In interactive mode: displays a new prompt on a new line
*/
void	handle_sigint(int sig)
{
	DEBUG_SIGNAL_MSG("Received SIGINT (signal %d)", sig);
	g_signal_received = sig;
	write(STDOUT_FILENO, "\n", 1);
	rl_on_new_line();
	rl_replace_line("", 0);
	rl_redisplay();
}

/*
** Signal handler for SIGQUIT (Ctrl-\)
** Does nothing in interactive mode
*/
void	handle_sigquit(int sig)
{
	DEBUG_SIGNAL_MSG("Received SIGQUIT (signal %d) - ignoring", sig);
	(void)sig;
}

/*
** Setup signal handlers for parent process (interactive mode)
** SIGINT: custom handler that displays new prompt
** SIGQUIT: ignored
*/
void	setup_signals(void)
{
	struct sigaction	sa_int;
	struct sigaction	sa_quit;

	DEBUG_SIGNAL_MSG("Setting up signal handlers for interactive mode");
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

/*
** Setup signal handlers for child process
** Reset to default behavior for proper command execution
*/
void	setup_child_signals(void)
{
	struct sigaction	sa;

	DEBUG_SIGNAL_MSG("Resetting signal handlers for child process");
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
	write(STDOUT_FILENO, "\n", 1);
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
