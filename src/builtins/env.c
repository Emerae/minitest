#include "../../includes/minishell.h"

/*
** Builtin env command
** Prints all environment variables
*/
int	builtin_env(char **env)
{
	int	i;

	if (!env)
		return (1);
	i = 0;
	while (env[i])
	{
		if (ft_strchr(env[i], '='))
		{
			write(STDOUT_FILENO, env[i], ft_strlen(env[i]));
			write(STDOUT_FILENO, "\n", 1);
		}
		i++;
	}
	return (0);
}
