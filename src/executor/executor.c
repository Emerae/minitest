#include "../../includes/minishell.h"

int	is_builtin(char *cmd)
{
	if (!cmd)
		return (0);
	if (ft_strcmp(cmd, "echo") == 0)
		return (1);
	if (ft_strcmp(cmd, "cd") == 0)
		return (1);
	if (ft_strcmp(cmd, "pwd") == 0)
		return (1);
	if (ft_strcmp(cmd, "export") == 0)
		return (1);
	if (ft_strcmp(cmd, "unset") == 0)
		return (1);
	if (ft_strcmp(cmd, "env") == 0)
		return (1);
	if (ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

int	execute_builtin(t_cmd *cmd, t_shell *shell)
{
	if (ft_strcmp(cmd->args[0], "echo") == 0)
		return (builtin_echo(cmd->args));
	if (ft_strcmp(cmd->args[0], "cd") == 0)
		return (builtin_cd(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "pwd") == 0)
		return (builtin_pwd());
	if (ft_strcmp(cmd->args[0], "export") == 0)
		return (builtin_export(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "unset") == 0)
		return (builtin_unset(cmd->args, &shell->env));
	if (ft_strcmp(cmd->args[0], "env") == 0)
		return (builtin_env(shell->env));
	if (ft_strcmp(cmd->args[0], "exit") == 0)
		return (builtin_exit(cmd->args, shell));
	return (0);
}

static void	execute_child_process(t_cmd *cmd, t_shell *shell)
{
	char	*cmd_path;
	char	**expanded_args;

	setup_child_signals();
	if (setup_redirections(cmd->redirs) == -1)
		exit(1);
	if (is_builtin(cmd->args[0]))
		exit(execute_builtin(cmd, shell));
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
	free(cmd_path);
	free_string_array(expanded_args);
	exit(ERROR_PERMISSION);
}

static int	wait_child_process(pid_t pid)
{
	int	status;

	status = 0;
	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (WEXITSTATUS(status));
}

static int	execute_external_command(t_cmd *cmd, t_shell *shell)
{
	pid_t	pid;
	int		result;

	pid = fork();
	if (pid == -1)
	{
		perror("minishell: fork");
		return (1);
	}
	if (pid == 0)
		execute_child_process(cmd, shell);
	shell->current_child_pid = pid;
	result = wait_child_process(pid);
	shell->current_child_pid = 0;
	return (result);
}

static int	must_run_in_parent(char *cmd)
{
	if (ft_strcmp(cmd, "export") == 0)
		return (1);
	if (ft_strcmp(cmd, "unset") == 0)
		return (1);
	if (ft_strcmp(cmd, "cd") == 0)
		return (1);
	if (ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

static int	execute_builtin_with_redirs(t_cmd *cmd, t_shell *shell)
{
	int	saved_stdout;
	int	saved_stdin;
	int	ret;

	saved_stdout = dup(STDOUT_FILENO);
	saved_stdin = dup(STDIN_FILENO);
	if (saved_stdout == -1 || saved_stdin == -1)
		return (1);
	if (setup_redirections(cmd->redirs) == -1)
	{
		close(saved_stdout);
		close(saved_stdin);
		return (1);
	}
	ret = execute_builtin(cmd, shell);
	dup2(saved_stdout, STDOUT_FILENO);
	dup2(saved_stdin, STDIN_FILENO);
	close(saved_stdout);
	close(saved_stdin);
	return (ret);
}

int	execute_simple_command(t_cmd *cmd, t_shell *shell)
{
	if (!cmd || !cmd->args || !cmd->args[0])
		return (0);
	if (is_builtin(cmd->args[0]))
	{
		if (cmd->redirs && must_run_in_parent(cmd->args[0]))
			return (execute_builtin_with_redirs(cmd, shell));
		if (cmd->redirs)
			return (execute_external_command(cmd, shell));
		return (execute_builtin(cmd, shell));
	}
	return (execute_external_command(cmd, shell));
}

int	execute_command_line(t_cmd *cmd_list, t_shell *shell)
{
	if (!cmd_list)
		return (0);
	if (!cmd_list->next)
		return (execute_simple_command(cmd_list, shell));
	return (execute_pipeline(cmd_list, shell));
}
