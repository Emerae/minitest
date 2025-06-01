#include "../../includes/minishell.h"

int	create_pipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == -1)
	{
		perror("minishell: pipe");
		return (-1);
	}
	return (0);
}

void	setup_pipe_child(int pipe_fd[2], int prev_pipe, int is_last)
{
	if (prev_pipe != -1)
	{
		dup2(prev_pipe, STDIN_FILENO);
		close(prev_pipe);
	}
	if (!is_last)
	{
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
	}
}

void	close_pipe_parent(int pipe_fd[2], int *prev_pipe)
{
	if (*prev_pipe != -1)
		close(*prev_pipe);
	if (pipe_fd && pipe_fd[0] != -1)
	{
		close(pipe_fd[1]);
		*prev_pipe = pipe_fd[0];
	}
}

static int	execute_external_in_child(t_cmd *cmd, t_shell *shell)
{
	char	*cmd_path;
	char	**expanded_args;

	expanded_args = expand_args_tildes(cmd->args, shell->env);
	if (!expanded_args)
		exit(1);
	cmd_path = find_command_path(expanded_args[0], shell->env);
	if (!cmd_path)
	{
		print_error(expanded_args[0], "command not found");
		free_string_array(expanded_args);
		exit(ERROR_CMD_NOT_FOUND);
	}
	execve(cmd_path, expanded_args, shell->env);
	perror("minishell: execve");
	free_string_array(expanded_args);
	exit(ERROR_PERMISSION);
}

static void	execute_child_process(t_cmd *cmd, t_shell *shell,
								int prev_pipe, int pipe_fd[2])
{
	setup_child_signals();
	shell->in_child = 1;
	setup_pipe_child(pipe_fd, prev_pipe, cmd->next == NULL);
	if (setup_redirections(cmd->redirs) == -1)
		exit(1);
	if (is_builtin(cmd->args[0]))
		exit(execute_builtin(cmd, shell));
	execute_external_in_child(cmd, shell);
}

static int	execute_piped_command(t_cmd *cmd, t_shell *shell,
								int prev_pipe, int pipe_fd[2])
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
		execute_child_process(cmd, shell, prev_pipe, pipe_fd);
	return (pid);
}

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

static int	preprocess_heredocs(t_cmd *cmd_list)
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

static int	count_commands(t_cmd *cmd_list)
{
	t_cmd	*current;
	int		count;

	count = 0;
	current = cmd_list;
	while (current)
	{
		count++;
		current = current->next;
	}
	return (count);
}

static int	setup_pipes_for_command(t_cmd *current, t_exec *exec)
{
	if (current->next && create_pipe(exec->pipe_fd) == -1)
		return (-1);
	return (0);
}

static int	*get_pipe_for_command(t_cmd *current, t_exec *exec)
{
	if (current->next)
		return (exec->pipe_fd);
	return (NULL);
}

static int	execute_and_manage_pipes(t_cmd *current, t_shell *shell,
									t_exec *exec, t_pipeline_data *data)
{
	int	*pipe_for_cmd;
	int	*pipe_for_close;

	pipe_for_cmd = get_pipe_for_command(current, exec);
	data->pids[data->index] = execute_piped_command(current, shell,
			exec->prev_pipe, pipe_for_cmd);
	if (data->pids[data->index] == -1)
		return (-1);
	pipe_for_close = get_pipe_for_command(current, exec);
	close_pipe_parent(pipe_for_close, &exec->prev_pipe);
	return (0);
}

static int	execute_pipeline_loop(t_cmd *cmd_list, t_shell *shell,
								t_exec *exec, pid_t *pids)
{
	t_cmd			*current;
	t_pipeline_data	data;

	current = cmd_list;
	data.pids = pids;
	data.index = 0;
	while (current)
	{
		if (setup_pipes_for_command(current, exec) == -1)
			return (-1);
		if (execute_and_manage_pipes(current, shell, exec, &data) == -1)
			return (-1);
		current = current->next;
		data.index++;
	}
	return (data.index);
}

static int	wait_all_processes(pid_t *pids, int cmd_count)
{
	int	i;
	int	status;

	i = 0;
	status = 0;
	while (i < cmd_count)
	{
		waitpid(pids[i], &status, 0);
		i++;
	}
	if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (WEXITSTATUS(status));
}

int	execute_pipeline(t_cmd *cmd_list, t_shell *shell)
{
	t_exec	exec;
	pid_t	*pids;
	int		cmd_count;
	int		result;

	if (preprocess_heredocs(cmd_list) == -1)
		return (1);
	cmd_count = count_commands(cmd_list);
	pids = malloc(sizeof(pid_t) * cmd_count);
	if (!pids)
		return (1);
	exec.prev_pipe = -1;
	if (execute_pipeline_loop(cmd_list, shell, &exec, pids) == -1)
	{
		free(pids);
		return (1);
	}
	result = wait_all_processes(pids, cmd_count);
	free(pids);
	return (result);
}
