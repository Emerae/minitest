#include "../prser.h"

static int	cy0_check_quote_2(char *s, int *i, char quote)
{
	int	flag;

	flag = 1;
	*i = *i + 1;
	while (s[*i])
	{
		if (s[*i] == quote)
		{
			flag = 0;
			*i = *i + 1;
			break ;
		}
		*i = *i + 1;
	}
	return (flag);
}

int	cy0_check_quote_1(char *s)
{
	int	i;
	int	flag;

	i = 0;
	flag = 0;
	while (s[i])
	{
		if (s[i] == '\'')
			flag = cy0_check_quote_2(s, &i, '\'');
		else if (s[i] == '"')
			flag = cy0_check_quote_2(s, &i, '"');
		else
			i = i + 1;
	}
	return (flag);
}
