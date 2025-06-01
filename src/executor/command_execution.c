#include "../../includes/minishell.h"

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

int	execute_and_manage_pipes(t_cmd *current, t_shell *shell,
								t_exec *exec, t_pipeline_data *data)
{
	int	*pipe_for_cmd;
	int	*pipe_for_close;

	if (current->next && create_pipe(exec->pipe_fd) == -1)
		return (-1);
	if (current->next)
		pipe_for_cmd = exec->pipe_fd;
	else
		pipe_for_cmd = NULL;
	data->pids[data->index] = execute_piped_command(current, shell,
			exec->prev_pipe, pipe_for_cmd);
	if (data->pids[data->index] == -1)
		return (-1);
	if (current->next)
		pipe_for_close = exec->pipe_fd;
	else
		pipe_for_close = NULL;
	close_pipe_parent(pipe_for_close, &exec->prev_pipe);
	return (0);
}
