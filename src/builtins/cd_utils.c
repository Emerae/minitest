#include "../../includes/minishell.h"

int	calculate_total_length(char **stack, int count)
{
	int	total_len;
	int	i;

	total_len = 1;
	i = 0;
	while (i < count)
	{
		total_len += ft_strlen(stack[i]) + 1;
		i++;
	}
	return (total_len);
}

void	build_path_string(char **stack, int count, char *result, int absolute)
{
	int	i;

	result[0] = '\0';
	if (absolute)
		ft_strcat(result, "/");
	i = 0;
	while (i < count)
	{
		ft_strcat(result, stack[i]);
		if (i < count - 1)
			ft_strcat(result, "/");
		i++;
	}
}

char	*rebuild_path(char **stack, int count, int absolute)
{
	char	*result;
	int		total_len;

	total_len = calculate_total_length(stack, count);
	result = malloc(total_len + 1);
	if (!result)
		return (NULL);
	build_path_string(stack, count, result, absolute);
	return (result);
}
