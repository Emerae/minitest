#include "../../includes/minishell.h"

/*
** Open file with appropriate flags based on redirection type
** Type 0: < (input)
** Type 1: > (output truncate)
** Type 2: >> (output append)
** Type 3: << (heredoc)
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


int	handle_heredoc(char *delimiter)
{
	int		pipe_fd[2];
	char	*line;
	
	if (pipe(pipe_fd) == -1)
		return (-1);
	setup_heredoc_signals();
	while (1)
	{
		line = readline("> ");
		if (!line || g_signal_received == SIGINT)
		{
			if (g_signal_received == SIGINT)
			{
				g_signal_received = 0;  // Nettoyer ici
				close(pipe_fd[1]);
				close(pipe_fd[0]);
				setup_signals();
				return (-1);
			}
			break ;
		}
		if (ft_strcmp(line, delimiter) == 0)
		{
			free(line);
			break ;
		}
		write(pipe_fd[1], line, ft_strlen(line));
		write(pipe_fd[1], "\n", 1);
		free(line);
	}
	close(pipe_fd[1]);
	setup_signals();
	return (pipe_fd[0]);
}

/*
** Handle heredoc redirection
** Read input until delimiter is found

int	handle_heredoc(char *delimiter)
{
	int		pipe_fd[2];
	char	*line;
	//size_t	delim_len;

	DEBUG_REDIR_MSG("Starting heredoc with delimiter '%s'", delimiter);
	
	if (pipe(pipe_fd) == -1)
	{
		DEBUG_ERROR("Failed to create pipe for heredoc");
		return (-1);
	}
	//delim_len = ft_strlen(delimiter);
	setup_heredoc_signals();
	while (1)
	{
		line = readline("> ");
		if (!line || g_signal_received == SIGINT)
		{
			if (g_signal_received == SIGINT)
			{
				DEBUG_SIGNAL_MSG("Heredoc interrupted by SIGINT");
				g_signal_received = 0;
			}
			else
				DEBUG_REDIR_MSG("Heredoc EOF reached");
			break ;
		}
		if (ft_strcmp(line, delimiter) == 0)
		{
			DEBUG_REDIR_MSG("Heredoc delimiter found");
			free(line);
			break ;
		}
		write(pipe_fd[1], line, ft_strlen(line));
		write(pipe_fd[1], "\n", 1);
		free(line);
	}
	close(pipe_fd[1]);
	setup_signals();
	DEBUG_REDIR_MSG("Heredoc completed, returning read fd %d", pipe_fd[0]);
	return (pipe_fd[0]);
}
*/

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
