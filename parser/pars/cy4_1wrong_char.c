#include "../prser.h"

static int	cy4_1wrong_char2(t_input *current)
{
	int	i;

	i = 0;
	while (current->input[i])
	{
		if (cy0_analyse_char2(current->input[i]) >= 1 && current->type == 2)
			return (1);
		if (cy0_analyse_char2(current->input[i]) == -14 && current->type == 2)
			return (2);
		if (cy0_analyse_char2(current->input[i]) == 2 && current->type == 4)
			return (3);
		i = i + 1;
	}
	return (0);
}

int	cy4_1wrong_char(t_input *head)
{
	t_input	*current;
	int		ret;

	current = head;
	while (current)
	{
		if (current->input)
		{
			ret = cy4_1wrong_char2(current);
			if (ret != 0)
				return (ret);
		}
		current = current->next;
	}
	return (0);
}
