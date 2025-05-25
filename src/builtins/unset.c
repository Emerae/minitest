#include "../../includes/minishell.h"

/*
** Check if variable name is valid for unset
*/
static int	is_valid_unset_identifier(char *str)
{
	int	i;

	if (!str || !*str)
		return (0);
	if (!((str[0] >= 'a' && str[0] <= 'z')
		|| (str[0] >= 'A' && str[0] <= 'Z')
		|| str[0] == '_'))
		return (0);
	i = 1;
	while (str[i])
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
** Builtin unset command
** Removes environment variables
*/
int	builtin_unset(char **args, char ***env)
{
	int	i;
	int	ret;

	if (!args[1])
		return (0);
	i = 1;
	ret = 0;
	while (args[i])
	{
		if (!is_valid_unset_identifier(args[i]))
		{
			write(STDERR_FILENO, "minishell: unset: `", 19);
			write(STDERR_FILENO, args[i], ft_strlen(args[i]));
			write(STDERR_FILENO, "': not a valid identifier\n", 26);
			ret = 1;
		}
		else
			unset_env_value(env, args[i]);
		i++;
	}
	return (ret);
}
