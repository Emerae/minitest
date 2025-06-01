#include "../../includes/minishell.h"

int	handle_heredoc_interruption(int pipe_fd[2])
{
	if (g_signal_received == SIGINT)
	{
		g_signal_received = 0;
		close(pipe_fd[1]);
		close(pipe_fd[0]);
		setup_signals();
		return (-1);
	}
	return (0);
}

int	process_heredoc_line(char *line, char *delimiter, int pipe_fd[2])
{
	if (ft_strcmp(line, delimiter) == 0)
	{
		free(line);
		return (1);
	}
	write(pipe_fd[1], line, ft_strlen(line));
	write(pipe_fd[1], "\n", 1);
	free(line);
	return (0);
}

int	read_heredoc_loop(char *delimiter, int pipe_fd[2])
{
	char	*line;
	int		line_result;

	while (1)
	{
		write(STDERR_FILENO, "> ", 2);
		line = readline("");
		if (!line || g_signal_received == SIGINT)
		{
			if (handle_heredoc_interruption(pipe_fd) == -1)
				return (-1);
			break ;
		}
		line_result = process_heredoc_line(line, delimiter, pipe_fd);
		if (line_result == 1)
			break ;
		if (line_result == -1)
			return (-1);
	}
	return (0);
}
