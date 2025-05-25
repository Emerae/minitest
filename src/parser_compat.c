#include "../includes/minishell.h"

/*
** Print input list for parser compatibility
** This function is needed by the parser but we provide a stub implementation
*/
void	print_input_list(t_input *head_input)
{
	t_input	*current;

	if (!DEBUG || !DEBUG_PARSER)
		return ;
	current = head_input;
	DEBUG_PARSER_MSG("Input list contents:");
	while (current)
	{
		if (current->input)
		{
			DEBUG_PARSER_MSG("[%d] input='%s', type=%d", 
				current->number, current->input, current->type);
		}
		else
		{
			DEBUG_PARSER_MSG("[%d] (null)", current->number);
		}
		current = current->next;
	}
}
