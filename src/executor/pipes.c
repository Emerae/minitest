#include "../../includes/minishell.h"

/*
** Create a pipe and handle errors
*/
int	create_pipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == -1)
	{
		perror("minishell: pipe");
		return (-1);
	}
	return (0);
}

/*
** Setup pipes for child process
** prev_pipe: read end of previous pipe (-1 if first command)
** pipe_fd: current pipe (write end for this command)
** is_last: 1 if last command in pipeline
*/
void	setup_pipe_child(int pipe_fd[2], int prev_pipe, int is_last)
{
	DEBUG_PIPE_MSG("Setting up pipes for child process");
	debug_print_pipe_state(prev_pipe, pipe_fd);
	
	if (prev_pipe != -1)
	{
		DEBUG_PIPE_MSG("Redirecting stdin from prev_pipe %d", prev_pipe);
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

/*
** Close pipes in parent after fork
*/
/*
** Close pipes in parent after fork
*/
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

/*
** Execute a command in a pipeline
*/
static int	execute_piped_command(t_cmd *cmd, t_shell *shell,
								int prev_pipe, int pipe_fd[2])
{
	pid_t	pid;
	char	*cmd_path;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		setup_child_signals();
		shell->in_child = 1;
		setup_pipe_child(pipe_fd, prev_pipe, cmd->next == NULL);
		if (setup_redirections(cmd->redirs) == -1)
			exit(1);
		if (is_builtin(cmd->args[0]))
			exit(execute_builtin(cmd, shell));
		cmd_path = find_command_path(cmd->args[0], shell->env);
		if (!cmd_path)
		{
			print_error(cmd->args[0], "command not found");
			exit(ERROR_CMD_NOT_FOUND);
		}
		execve(cmd_path, cmd->args, shell->env);
		perror("minishell: execve");
		exit(ERROR_PERMISSION);
	}
	return (pid);
}

/*
** Create unique temporary filename using static counter
*/
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
		/* Convert number to string manually */
		if (temp >= 10)
			buffer[i++] = (temp / 10) + '0';
		buffer[i++] = (temp % 10) + '0';
	}
	buffer[i] = '\0';
}

/*
** Preprocess all heredocs in a pipeline
*/
static int	preprocess_heredocs(t_cmd *cmd_list)
{
	t_cmd		*current;
	t_redir		*redir;
	char		*line;
	char		temp_filename[64];
	int			temp_fd;
	static int	heredoc_count = 0;

	current = cmd_list;
	while (current)
	{
		redir = current->redirs;
		while (redir)
		{
			if (redir->type == 3)
			{
				create_temp_filename(temp_filename, heredoc_count);
				heredoc_count++;
				temp_fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
				if (temp_fd == -1)
					return (-1);
				setup_heredoc_signals();
				while (1)
				{
					line = readline("> ");
					if (!line || g_signal_received == SIGINT)
					{
						if (g_signal_received == SIGINT)
						{
							printf("DEBUG: SIGINT detected, cleaning up\n");  // Debug temporaire
							g_signal_received = 0;
							close(temp_fd);
							unlink(temp_filename);
							setup_signals();
							return (-1);
						}
						break ;
					}
					if (ft_strcmp(line, redir->file) == 0)
					{
						free(line);
						break ;
					}
					write(temp_fd, line, ft_strlen(line));
					write(temp_fd, "\n", 1);
					free(line);
				}
				close(temp_fd);
				setup_signals();
				free(redir->file);
				redir->file = ft_strdup(temp_filename);
				redir->type = 0;
			}
			redir = redir->next;
		}
		current = current->next;
	}
	return (0);
}


int	execute_pipeline(t_cmd *cmd_list, t_shell *shell)
{
	t_exec	exec;
	t_cmd	*current;
	pid_t	*pids;
	int		cmd_count;
	int		i;
	int		*pipe_for_cmd;
	int		*pipe_for_close;

	if (preprocess_heredocs(cmd_list) == -1)
		return (1);
	cmd_count = 0;
	current = cmd_list;
	while (current && ++cmd_count)
		current = current->next;
	pids = malloc(sizeof(pid_t) * cmd_count);
	if (!pids)
		return (1);
	exec.prev_pipe = -1;
	current = cmd_list;
	i = 0;
	while (current)
	{
		if (current->next && create_pipe(exec.pipe_fd) == -1)
		{
			free(pids);
			return (1);
		}
		if (current->next)
			pipe_for_cmd = exec.pipe_fd;
		else
			pipe_for_cmd = NULL;
		pids[i] = execute_piped_command(current, shell, exec.prev_pipe, pipe_for_cmd);
		if (pids[i] == -1)
		{
			free(pids);
			return (1);
		}
		if (current->next)
			pipe_for_close = exec.pipe_fd;
		else
			pipe_for_close = NULL;
		close_pipe_parent(pipe_for_close, &exec.prev_pipe);
		current = current->next;
		i++;
	}
	i = 0;
	exec.status = 0;
	while (i < cmd_count)
	{
		waitpid(pids[i], &exec.status, 0);
		i++;
	}
	free(pids);
	if (WIFSIGNALED(exec.status))
		return (128 + WTERMSIG(exec.status));
	return (WEXITSTATUS(exec.status));
}


/*
** Execute a pipeline of commands

int	execute_pipeline(t_cmd *cmd_list, t_shell *shell)
{
	t_exec	exec;
	t_cmd	*current;
	pid_t	*pids;
	int		cmd_count;
	int		i;

	cmd_count = 0;
	current = cmd_list;
	while (current && ++cmd_count)
		current = current->next;
	pids = malloc(sizeof(pid_t) * cmd_count);
	if (!pids)
		return (1);
	exec.prev_pipe = -1;
	current = cmd_list;
	i = 0;
	while (current)
	{
		if (current->next && create_pipe(exec.pipe_fd) == -1)
		{
			free(pids);
			return (1);
		}
		pids[i] = execute_piped_command(current, shell,
			exec.prev_pipe, current->next ? exec.pipe_fd : NULL);
		if (pids[i] == -1)
		{
			free(pids);
			return (1);
		}
		close_pipe_parent(current->next ? exec.pipe_fd : NULL, &exec.prev_pipe);
		current = current->next;
		i++;
	}
	i = 0;
	exec.status = 0;
	while (i < cmd_count)
	{
		waitpid(pids[i], &exec.status, 0);
		i++;
	}
	free(pids);
	if (WIFSIGNALED(exec.status))
		return (128 + WTERMSIG(exec.status));
	return (WEXITSTATUS(exec.status));
}
*/
