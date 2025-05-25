#include "../../includes/minishell.h"

/*
** Count number of digits in number
*/
static int	count_digits(int n)
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

/*
** Get absolute value safely (handle INT_MIN)
*/
static unsigned int	get_abs_value(int n)
{
	if (n < 0)
		return (-(unsigned int)n);
	return ((unsigned int)n);
}

/*
** Convert integer to string
*/
char	*ft_itoa(int n)
{
	char			*str;
	int				len;
	unsigned int	num;

	len = count_digits(n);
	str = malloc(len + 1);
	if (!str)
		return (NULL);
	str[len] = '\0';
	if (n < 0)
		str[0] = '-';
	num = get_abs_value(n);
	if (num == 0)
		str[0] = '0';
	while (num > 0)
	{
		len--;
		str[len] = (num % 10) + '0';
		num /= 10;
	}
	return (str);
}
