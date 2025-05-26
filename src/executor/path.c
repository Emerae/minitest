#include "../../includes/minishell.h"

/*
** Check if file exists and is executable
*/
static int	is_executable(char *path)
{
	struct stat	statbuf;

	DEBUG_PATH_MSG("Checking if '%s' is executable", path);
	
	if (stat(path, &statbuf) == -1)
	{
		DEBUG_TRACE("stat failed for '%s'", path);
		return (0);
	}
	if (S_ISREG(statbuf.st_mode) && (statbuf.st_mode & S_IXUSR))
	{
		DEBUG_PATH_MSG("'%s' is executable", path);
		return (1);
	}
	DEBUG_TRACE("'%s' is not executable", path);
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
** Find command in PATH or as absolute/relative path
*/
char	*find_command_path(char *cmd, char **env)
{
	char		**paths;
	char		*cmd_path;
	int			i;
	struct stat	statbuf;

	DEBUG_PATH_MSG("Searching for command '%s'", cmd);
	
	if (!cmd || !*cmd)
		return (NULL);
	if (ft_strchr(cmd, '/'))
	{
		DEBUG_PATH_MSG("Command contains '/', checking as absolute/relative path");
		/* Pour les chemins absolus/relatifs, vérifier seulement l'existence
		 * Si le fichier existe, retourner le chemin même s'il n'est pas exécutable
		 * execve() se chargera de l'erreur de permission appropriée */
		if (stat(cmd, &statbuf) == -1)
		{
			DEBUG_PATH_MSG("File '%s' does not exist", cmd);
			return (NULL);  /* Fichier inexistant → command not found */
		}
		if (S_ISREG(statbuf.st_mode))
		{
			DEBUG_PATH_MSG("File '%s' exists, returning path", cmd);
			return (ft_strdup(cmd));  /* Fichier existe → laisser execve gérer */
		}
		DEBUG_PATH_MSG("'%s' is not a regular file", cmd);
		return (NULL);
	}
	/* Reste du code inchangé pour les commandes dans PATH */
	paths = get_paths_from_env(env);
	if (!paths)
	{
		DEBUG_ERROR("No PATH found in environment");
		return (NULL);
	}
	i = 0;
	while (paths[i])
	{
		DEBUG_TRACE("Checking in directory: %s", paths[i]);
		cmd_path = check_path(paths[i], cmd);
		if (cmd_path)
		{
			DEBUG_PATH_MSG("Command found: %s", cmd_path);
			free_string_array(paths);
			return (cmd_path);
		}
		i++;
	}
	DEBUG_PATH_MSG("Command '%s' not found in PATH", cmd);
	free_string_array(paths);
	return (NULL);
}

/*
** Find command in PATH or as absolute/relative path

char	*find_command_path(char *cmd, char **env)
{
	char	**paths;
	char	*cmd_path;
	int		i;

	DEBUG_PATH_MSG("Searching for command '%s'", cmd);
	
	if (!cmd || !*cmd)
		return (NULL);
	if (ft_strchr(cmd, '/'))
	{
		DEBUG_PATH_MSG("Command contains '/', checking as absolute/relative path");
		if (is_executable(cmd))
			return (ft_strdup(cmd));
		return (NULL);
	}
	paths = get_paths_from_env(env);
	if (!paths)
	{
		DEBUG_ERROR("No PATH found in environment");
		return (NULL);
	}
	i = 0;
	while (paths[i])
	{
		DEBUG_TRACE("Checking in directory: %s", paths[i]);
		cmd_path = check_path(paths[i], cmd);
		if (cmd_path)
		{
			DEBUG_PATH_MSG("Command found: %s", cmd_path);
			free_string_array(paths);
			return (cmd_path);
		}
		i++;
	}
	DEBUG_PATH_MSG("Command '%s' not found in PATH", cmd);
	free_string_array(paths);
	return (NULL);
}
*/

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
