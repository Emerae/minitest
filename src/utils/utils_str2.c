#include "../../includes/minishell.h"

/*
** Copy n characters from src to dst
*/
char	*ft_strncpy(char *dst, const char *src, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n && src[i])
	{
		dst[i] = src[i];
		i++;
	}
	while (i < n)
	{
		dst[i] = '\0';
		i++;
	}
	return (dst);
}

/*
** Duplicate string array
*/
char	**duplicate_string_array(char **array)
{
	char	**dup;
	int		count;
	int		i;

	if (!array)
		return (NULL);
	count = count_string_array(array);
	dup = malloc(sizeof(char *) * (count + 1));
	if (!dup)
		return (NULL);
	i = 0;
	while (i < count)
	{
		dup[i] = ft_strdup(array[i]);
		if (!dup[i])
		{
			free_string_array(dup);
			return (NULL);
		}
		i++;
	}
	dup[i] = NULL;
	return (dup);
}
