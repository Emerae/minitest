#include "../../includes/minishell.h"

int	is_space_char(char c)
{
	return (c == ' ' || c == '\t' || c == '\n'
		|| c == '\v' || c == '\f' || c == '\r');
}

int	count_digits(int n)
{
	int	count;

	count = 0;
	if (n == 0)
		return (1);
	if (n < 0)
		count++;
	while (n != 0)
	{
		n /= 10;
		count++;
	}
	return (count);
}

unsigned int	get_abs_value(int n)
{
	if (n < 0)
		return (-(unsigned int)n);
	return ((unsigned int)n);
}
