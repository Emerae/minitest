#include "../../includes/minishell.h"

/*
** Update PWD and OLDPWD environment variables
*/
static int	update_pwd_env(char ***env, char *old_pwd)
{
	char	new_pwd[4096];

	if (!getcwd(new_pwd, sizeof(new_pwd)))
		return (1);
	if (old_pwd)
		set_env_value(env, "OLDPWD", old_pwd);
	set_env_value(env, "PWD", new_pwd);
	return (0);
}

/*
** Validate arguments and determine target path
*/
static char	*get_target_path(char **args, char **env)
{
	char	*home;

	if (!args[1])
	{
		home = get_env_value(env, "HOME");
		if (!home)
		{
			write(STDERR_FILENO, "minishell: cd: HOME not set\n", 28);
			return (NULL);
		}
		return (home);
	}
	if (args[2])
	{
		write(STDERR_FILENO, "minishell: cd: too many arguments\n", 34);
		return (NULL);
	}
	return (args[1]);
}

/*
** Save current working directory
*/
static int	save_current_pwd(char *old_pwd, int size)
{
	if (!getcwd(old_pwd, size))
	{
		old_pwd[0] = '\0';
		return (0);
	}
	return (1);
}

/*
** Change to target directory with error handling
*/
static int	change_directory(char *path)
{
	if (chdir(path) == -1)
	{
		write(STDERR_FILENO, "minishell: cd: ", 15);
		write(STDERR_FILENO, path, ft_strlen(path));
		write(STDERR_FILENO, ": ", 2);
		perror("");
		return (1);
	}
	return (0);
}

/*
** Builtin cd command
*/
int	builtin_cd(char **args, char ***env)
{
	char	*path;
	char	old_pwd[4096];
	int		has_old_pwd;

	path = get_target_path(args, *env);
	if (!path)
		return (1);
	has_old_pwd = save_current_pwd(old_pwd, sizeof(old_pwd));
	if (change_directory(path))
		return (1);
	if (has_old_pwd)
		return (update_pwd_env(env, old_pwd));
	return (update_pwd_env(env, NULL));
}