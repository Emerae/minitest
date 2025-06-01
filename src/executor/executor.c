/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:39:28 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:39:28 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

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
