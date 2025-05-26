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
** Builtin cd command
** Changes directory to path specified
*/
int	builtin_cd(char **args, char ***env)
{
	char	*path;
	char	*home;
	char	old_pwd[4096];

	if (!args[1])
	{
		home = get_env_value(*env, "HOME");
		if (!home)
		{
			write(STDERR_FILENO, "minishell: cd: HOME not set\n", 28);
			return (1);
		}
		path = home;
	}
	else if (args[2])
	{
		write(STDERR_FILENO, "minishell: cd: too many arguments\n", 34);
		return (1);
	}
	else
		path = args[1];
	if (!getcwd(old_pwd, sizeof(old_pwd)))
		old_pwd[0] = '\0';
	if (chdir(path) == -1)
	{
		write(STDERR_FILENO, "minishell: cd: ", 15);
		write(STDERR_FILENO, path, ft_strlen(path));
		write(STDERR_FILENO, ": ", 2);
		perror("");
		return (1);
	}
	return (update_pwd_env(env, old_pwd[0] ? old_pwd : NULL));
}
