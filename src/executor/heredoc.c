#include "../../includes/minishell.h"

static void	create_temp_filename(char *buffer, int counter)
{
	char	*base;
	int		i;
	int		temp;

	base = "/tmp/minishell_heredoc_";
	i = 0;
	while (base[i])
	{
		buffer[i] = base[i];
		i++;
	}
	temp = counter;
	if (temp == 0)
		buffer[i++] = '0';
	else
	{
		if (temp >= 10)
			buffer[i++] = (temp / 10) + '0';
		buffer[i++] = (temp % 10) + '0';
	}
	buffer[i] = '\0';
}

static int	handle_heredoc_signal_cleanup(int temp_fd, char *temp_filename)
{
	if (g_signal_received == SIGINT)
	{
		g_signal_received = 0;
		close(temp_fd);
		unlink(temp_filename);
		setup_signals();
		return (-1);
	}
	return (0);
}

static int	process_heredoc_lines(int temp_fd, char *delimiter,
								char *temp_filename)
{
	char	*line;

	while (1)
	{
		line = readline("> ");
		if (!line || g_signal_received == SIGINT)
		{
			if (handle_heredoc_signal_cleanup(temp_fd, temp_filename) == -1)
				return (-1);
			break ;
		}
		if (ft_strcmp(line, delimiter) == 0)
		{
			free(line);
			break ;
		}
		write(temp_fd, line, ft_strlen(line));
		write(temp_fd, "\n", 1);
		free(line);
	}
	return (0);
}

static int	process_single_heredoc(t_redir *redir, char *temp_filename,
								int *heredoc_count)
{
	int	temp_fd;

	create_temp_filename(temp_filename, *heredoc_count);
	(*heredoc_count)++;
	temp_fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (temp_fd == -1)
		return (-1);
	setup_heredoc_signals();
	if (process_heredoc_lines(temp_fd, redir->file, temp_filename) == -1)
		return (-1);
	close(temp_fd);
	setup_signals();
	free(redir->file);
	redir->file = ft_strdup(temp_filename);
	redir->type = 0;
	return (0);
}

int	preprocess_heredocs(t_cmd *cmd_list)
{
	t_cmd		*current;
	t_redir		*redir;
	char		temp_filename[64];
	static int	heredoc_count = 0;

	current = cmd_list;
	while (current)
	{
		redir = current->redirs;
		while (redir)
		{
			if (redir->type == 3)
			{
				if (process_single_heredoc(redir, temp_filename,
						&heredoc_count) == -1)
					return (-1);
			}
			redir = redir->next;
		}
		current = current->next;
	}
	return (0);
}
