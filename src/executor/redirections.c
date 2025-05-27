#include "../../includes/minishell.h"

/*
** Open file with appropriate flags based on redirection type
*/
int	open_file_for_redirect(char *filename, int type)
{
	int	fd;

	if (type == 0)
		fd = open(filename, O_RDONLY);
	else if (type == 1)
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (type == 2)
		fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		return (-1);
	if (fd == -1)
	{
		write(STDERR_FILENO, "minishell: ", 11);
		write(STDERR_FILENO, filename, ft_strlen(filename));
		write(STDERR_FILENO, ": ", 2);
		perror("");
	}
	return (fd);
}

/*
** Handle interruption in heredoc
*/
static int	handle_heredoc_interruption(int pipe_fd[2])
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

/*
** Process a single line in heredoc
*/
static int	process_heredoc_line(char *line, char *delimiter, int pipe_fd[2])
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

/*
** Read heredoc input until delimiter or interruption
*/
static int	read_heredoc_loop(char *delimiter, int pipe_fd[2])
{
	char	*line;
	int		line_result;

	while (1)
	{
		line = readline("> ");
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

/*
** Handle heredoc redirection
*/
int	handle_heredoc(char *delimiter)
{
	int	pipe_fd[2];

	if (pipe(pipe_fd) == -1)
		return (-1);
	setup_heredoc_signals();
	if (read_heredoc_loop(delimiter, pipe_fd) == -1)
		return (-1);
	close(pipe_fd[1]);
	setup_signals();
	return (pipe_fd[0]);
}

/*
** Apply a single redirection
*/
static int	apply_redirection(t_redir *redir)
{
	int	fd;
	int	target_fd;

	if (redir->type == 3)
		fd = handle_heredoc(redir->file);
	else
		fd = open_file_for_redirect(redir->file, redir->type);
	if (fd == -1)
		return (-1);
	if (redir->type == 0 || redir->type == 3)
		target_fd = STDIN_FILENO;
	else
		target_fd = STDOUT_FILENO;
	if (dup2(fd, target_fd) == -1)
	{
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

/*
** Setup all redirections for a command
*/
int	setup_redirections(t_redir *redirs)
{
	t_redir	*current;

	current = redirs;
	while (current)
	{
		if (apply_redirection(current) == -1)
			return (-1);
		current = current->next;
	}
	return (0);
}

