#include "../../includes/minishell.h"

/*
** Check if command is a builtin
*/
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

/*
** Execute a builtin command
*/
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

/*
** Execute external command with fork and execve
*/
static int	execute_external_command(t_cmd *cmd, t_shell *shell)
{
	pid_t	pid;
	int		status;
	char	*cmd_path;

	pid = fork();
	if (pid == -1)
	{
		perror("minishell: fork");
		return (1);
	}
	if (pid == 0)
	{
		setup_child_signals();
		if (setup_redirections(cmd->redirs) == -1)
			exit(1);
		cmd_path = find_command_path(cmd->args[0], shell->env);
		if (!cmd_path)
		{
			print_error(cmd->args[0], "command not found");
			exit(ERROR_CMD_NOT_FOUND);
		}
		execve(cmd_path, cmd->args, shell->env);
		perror("minishell: execve");
		free(cmd_path);
		exit(ERROR_PERMISSION);
	}
	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (WEXITSTATUS(status));
}

/*
** Execute a simple command (no pipes)
*/
int	execute_simple_command(t_cmd *cmd, t_shell *shell)
{
	if (!cmd || !cmd->args || !cmd->args[0])
		return (0);
	if (is_builtin(cmd->args[0]))
	{
		if (setup_redirections(cmd->redirs) == -1)
			return (1);
		return (execute_builtin(cmd, shell));
	}
	return (execute_external_command(cmd, shell));
}

/*
** Main execution function - dispatch based on command type
*/
int	execute_command_line(t_cmd *cmd_list, t_shell *shell)
{
	DEBUG_EXEC_MSG("Starting command execution");
	
	if (!cmd_list)
		return (0);
	
	if (!cmd_list->next)
	{
		DEBUG_EXEC_MSG("Single command detected");
		debug_print_cmd(cmd_list);
		return (execute_simple_command(cmd_list, shell));
	}
	
	DEBUG_EXEC_MSG("Pipeline detected");
	return (execute_pipeline(cmd_list, shell));
}
