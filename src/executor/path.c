#include "../../includes/minishell.h"

/*
** Check if file exists and is executable
*/
static int	is_executable(char *path)
{
	struct stat	statbuf;
	
	if (stat(path, &statbuf) == -1)
		return (0);
	if (S_ISREG(statbuf.st_mode) && (statbuf.st_mode & S_IXUSR))
		return (1);
	return (0);
}

/*
** Build full path and check if executable
*/
static char	*check_path(char *dir, char *cmd)
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

/*
** Get PATH directories from environment
*/
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

/*
** Handle absolute or relative path commands
*/
static char	*handle_absolute_path(char *cmd)
{
	struct stat	statbuf;

	if (stat(cmd, &statbuf) == -1)
		return (NULL);
	if (S_ISREG(statbuf.st_mode))
		return (ft_strdup(cmd));
	return (NULL);
}

/*
** Search command in PATH directories
*/
static char	*search_in_path(char *cmd, char **env)
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

/*
** Find command in PATH or as absolute/relative path
*/
char	*find_command_path(char *cmd, char **env)
{
	if (!cmd || !*cmd)
		return (NULL);
	if (ft_strchr(cmd, '/'))
		return (handle_absolute_path(cmd));
	return (search_in_path(cmd, env));
}

/*
** Print command error message
*/
void	print_error(char *cmd, char *msg)
{
	write(STDERR_FILENO, "minishell: ", 11);
	write(STDERR_FILENO, cmd, ft_strlen(cmd));
	write(STDERR_FILENO, ": ", 2);
	write(STDERR_FILENO, msg, ft_strlen(msg));
	write(STDERR_FILENO, "\n", 1);
}
