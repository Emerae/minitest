
#include "../../includes/minishell.h"

static int	process_segment(char *segment, char **stack, int *count,
					int absolute)
{
	if (!segment || segment[0] == '\0' || ft_strcmp(segment, ".") == 0)
		return (0);
	if (ft_strcmp(segment, "..") == 0)
	{
		if (*count > 0)
		{
			free(stack[*count - 1]);
			(*count)--;
		}
		else if (!absolute)
		{
			stack[*count] = ft_strdup("..");
			if (!stack[*count])
				return (-1);
			(*count)++;
		}
		return (0);
	}
	stack[*count] = ft_strdup(segment);
	if (!stack[*count])
		return (-1);
	(*count)++;
	return (0);
}

static int	process_segments_loop(char **segments, char **stack,
							int *count, int absolute)
{
	int	i;

	i = 0;
	while (segments[i])
	{
		if (process_segment(segments[i], stack, count, absolute) == -1)
			return (-1);
		i++;
	}
	return (0);
}

static void	cleanup_stack(char **stack, int count)
{
	while (count > 0)
		free(stack[--count]);
}

char	*normalize_path_segments(char *path)
{
	char	**segments;
	char	*stack[256];
	int		count;
	int		absolute;
	char	*result;

	absolute = (path[0] == '/');
	segments = ft_split(path, '/');
	if (!segments)
		return (NULL);
	count = 0;
	if (process_segments_loop(segments, stack, &count, absolute) == -1)
	{
		free_string_array(segments);
		return (NULL);
	}
	result = rebuild_path(stack, count, absolute);
	free_string_array(segments);
	cleanup_stack(stack, count);
	return (result);
}

char	*normalize_path(char *path, char **env)
{
	char	*expanded;
	char	*normalized;

	expanded = expand_single_tilde(path, env);
	if (!expanded)
		return (NULL);
	normalized = normalize_path_segments(expanded);
	free(expanded);
	return (normalized);
}
