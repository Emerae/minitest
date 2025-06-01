#include "../prser.h"

void	cy0_free_input_list(t_input *head)
{
	t_input	*current;
	t_input	*next;

	if (!head)
		return ;
	while (head->prev)
		head = head->prev;
	current = head;
	while (current != NULL)
	{
		if (current->input)
			free(current->input);
		if (current->input_type)
			free(current->input_type);
		if (current->input_num)
			free(current->input_num);
		next = current->next;
		free(current);
		current = next;
	}
}

void	cy0_free_cmd_list(t_cmd *cmd)
{
	t_cmd	*tmp_cmd;
	t_redir	*tmp_redir;
	int		i;

	while (cmd)
	{
		tmp_cmd = cmd->next;
		if (cmd->args)
		{
			i = 0;
			while (cmd->args[i])
				free(cmd->args[i++]);
			free(cmd->args);
		}
		while (cmd->redirs)
		{
			tmp_redir = cmd->redirs->next;
			free(cmd->redirs->file);
			free(cmd->redirs);
			cmd->redirs = tmp_redir;
		}
		free(cmd);
		cmd = tmp_cmd;
	}
}
