#include "../../includes/minishell.h"

char	*search_in_path(char *cmd, char **env)
{
	char	**paths;
	char	*cmd_path;
	int		i;

	paths = get_paths_from_env(env);
	if (!paths)
		return (NULL);
	i = 0;
	while (paths[i])
	{
		cmd_path = check_path(paths[i], cmd);
		if (cmd_path)
		{
			free_string_array(paths);
			return (cmd_path);
		}
		i++;
	}
	free_string_array(paths);
	return (NULL);
}
