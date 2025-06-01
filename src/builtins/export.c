#include "../../includes/minishell.h"

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

static int	validate_and_export(char ***env, char *arg, char *key, char *value)
{
	if (!is_valid_identifier(key))
	{
		print_export_error(arg);
		return (1);
	}
	set_env_value(env, key, value);
	return (0);
}

static int	export_var(char ***env, char *arg)
{
	char	*equal;
	char	*key;
	char	*value;
	int		result;

	if (parse_export_arg(arg, &key, &value, &equal))
		return (1);
	result = validate_and_export(env, arg, key, value);
	if (equal)
		free(key);
	return (result);
}

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
