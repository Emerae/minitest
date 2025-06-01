#include "../../includes/minishell.h"

static int	is_executable(char *path)
{
	struct stat	statbuf;

	if (stat(path, &statbuf) == -1)
		return (0);
	if (S_ISREG(statbuf.st_mode) && (statbuf.st_mode & S_IXUSR))
		return (1);
	return (0);
}

char	*check_path(char *dir, char *cmd)
{
	char	*full_path;
	int		dir_len;
	int		cmd_len;

	dir_len = ft_strlen(dir);
	cmd_len = ft_strlen(cmd);
	full_path = malloc(dir_len + cmd_len + 2);
	if (!full_path)
		return (NULL);
	ft_strcpy(full_path, dir);
	if (dir[dir_len - 1] != '/')
		ft_strcat(full_path, "/");
	ft_strcat(full_path, cmd);
	if (is_executable(full_path))
		return (full_path);
	free(full_path);
	return (NULL);
}

char	**get_paths_from_env(char **env)
{
	char	*path_value;
	char	**paths;

	path_value = get_env_value(env, "PATH");
	if (!path_value)
		return (NULL);
	paths = ft_split(path_value, ':');
	return (paths);
}

static char	*handle_absolute_path(char *cmd)
{
	struct stat	statbuf;

	if (stat(cmd, &statbuf) == -1)
		return (NULL);
	if (S_ISREG(statbuf.st_mode))
		return (ft_strdup(cmd));
	return (NULL);
}

char	*find_command_path(char *cmd, char **env)
{
	if (!cmd || !*cmd)
		return (NULL);
	if (ft_strchr(cmd, '/'))
		return (handle_absolute_path(cmd));
	return (search_in_path(cmd, env));
}
