#include "../../includes/minishell.h"

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
