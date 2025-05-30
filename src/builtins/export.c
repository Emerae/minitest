#include "../../includes/minishell.h"

/*
** Check if variable name is valid
*/
static int	is_valid_identifier(char *str)
{
	int	i;

	if (!str || !*str)
		return (0);
	if (!((str[0] >= 'a' && str[0] <= 'z')
		|| (str[0] >= 'A' && str[0] <= 'Z')
		|| str[0] == '_'))
		return (0);
	i = 1;
	while (str[i] && str[i] != '=')
	{
		if (!((str[i] >= 'a' && str[i] <= 'z')
			|| (str[i] >= 'A' && str[i] <= 'Z')
			|| (str[i] >= '0' && str[i] <= '9')
			|| str[i] == '_'))
			return (0);
		i++;
	}
	return (1);
}

/*
** Sort environment array using bubble sort
*/
static void	sort_env_array(char **env)
{
	int		i;
	int		j;
	char	*temp;

	i = 0;
	while (env[i])
	{
		j = i + 1;
		while (env[j])
		{
			if (ft_strcmp(env[i], env[j]) > 0)
			{
				temp = env[i];
				env[i] = env[j];
				env[j] = temp;
			}
			j++;
		}
		i++;
	}
}

/*
** Print sorted environment variables with declare -x prefix
*/
static void	print_env_variables(char **env)
{
	int	i;

	i = 0;
	while (env[i])
	{
		write(STDOUT_FILENO, "declare -x ", 11);
		write(STDOUT_FILENO, env[i], ft_strlen(env[i]));
		write(STDOUT_FILENO, "\n", 1);
		i++;
	}
}

/*
** Print all exported variables (sorted)
*/
static void	print_sorted_env(char **env)
{
	sort_env_array(env);
	print_env_variables(env);
}

/*
** Parse argument into key and value components
*/
static int	parse_export_arg(char *arg, char **key, char **value, char **equal)
{
	int	key_len;

	*equal = ft_strchr(arg, '=');
	if (!*equal)
	{
		*key = arg;
		*value = "";
		return (0);
	}
	key_len = *equal - arg;
	*key = malloc(key_len + 1);
	if (!*key)
		return (1);
	ft_strncpy(*key, arg, key_len);
	(*key)[key_len] = '\0';
	*value = *equal + 1;
	return (0);
}

/*
** Print export error message
*/
static void	print_export_error(char *arg)
{
	write(STDERR_FILENO, "minishell: export: `", 20);
	write(STDERR_FILENO, arg, ft_strlen(arg));
	write(STDERR_FILENO, "': not a valid identifier\n", 26);
}

/*
** Validate identifier and handle error
*/
static int	validate_and_export(char ***env, char *arg, char *key,
								char *value, char *equal)
{
	if (!is_valid_identifier(key))
	{
		print_export_error(arg);
		if (equal)
			free(key);
		return (1);
	}
	set_env_value(env, key, value);
	if (equal)
		free(key);
	return (0);
}

/*
** Export a variable
*/
static int	export_var(char ***env, char *arg)
{
	char	*equal;
	char	*key;
	char	*value;

	if (parse_export_arg(arg, &key, &value, &equal))
		return (1);
	return (validate_and_export(env, arg, key, value, equal));
}

/*
** Builtin export command
*/
int	builtin_export(char **args, char ***env)
{
	char	**env_copy;
	int		i;
	int		ret;

	if (!args[1])
	{
		env_copy = duplicate_string_array(*env);
		if (!env_copy)
			return (1);
		print_sorted_env(env_copy);
		free_string_array(env_copy);
		return (0);
	}
	i = 1;
	ret = 0;
	while (args[i])
	{
		if (export_var(env, args[i]) != 0)
			ret = 1;
		i++;
	}
	return (ret);
}
