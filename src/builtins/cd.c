#include "../../includes/minishell.h"

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
		return (ft_strdup(home));
	}
	if (args[2])
	{
		write(STDERR_FILENO, "minishell: cd: too many arguments\n", 34);
		return (NULL);
	}
	if (ft_strcmp(args[1], "..") == 0 || ft_strcmp(args[1], ".") == 0)
		return (ft_strdup(args[1]));
	if (ft_strstr(args[1], ".."))
		return (expand_single_tilde(args[1], env));
	return (normalize_path(args[1], env));
}

static int	save_current_pwd(char *old_pwd, int size)
{
	if (!getcwd(old_pwd, size))
	{
		old_pwd[0] = '\0';
		return (0);
	}
	return (1);
}

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
	{
		free(path);
		return (1);
	}
	free(path);
	if (has_old_pwd)
		return (update_pwd_env(env, old_pwd));
	return (update_pwd_env(env, NULL));
}
