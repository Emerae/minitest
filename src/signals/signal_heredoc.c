#include "../../includes/minishell.h"

void	handle_sigint_heredoc(int sig)
{
	g_signal_received = sig;
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
