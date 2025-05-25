#include "../../includes/minishell.h"

/*
** Setup terminal for minishell
** Disables ECHOCTL to prevent ^C from being displayed
*/
void	setup_terminal(t_shell *shell)
{
	struct termios	new_termios;

	tcgetattr(STDIN_FILENO, &shell->orig_termios);
	new_termios = shell->orig_termios;
	new_termios.c_lflag &= ~ECHOCTL;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

/*
** Restore terminal attributes
*/
void	restore_terminal_attrs(t_shell *shell)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &shell->orig_termios);
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
