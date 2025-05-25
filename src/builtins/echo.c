#include "../../includes/minishell.h"

/*
** Builtin echo command
** Supports -n option to not print newline
*/
int	builtin_echo(char **args)
{
	int	i;
	int	newline;

	DEBUG_BUILTIN_MSG("ECHO: starting");
	i = 1;
	newline = 1;
	if (args[1] && ft_strcmp(args[1], "-n") == 0)
	{
		DEBUG_BUILTIN_MSG("ECHO: -n flag detected");
		newline = 0;
		i = 2;
	}
	while (args[i])
	{
		write(STDOUT_FILENO, args[i], ft_strlen(args[i]));
		if (args[i + 1])
			write(STDOUT_FILENO, " ", 1);
		i++;
	}
	if (newline)
		write(STDOUT_FILENO, "\n", 1);
	return (0);
}
