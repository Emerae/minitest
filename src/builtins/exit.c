#include "../../includes/minishell.h"

/*
** Check if string is numeric
*/
static int	is_numeric(char *str)
{
	int	i;

	i = 0;
	if (str[i] == '-' || str[i] == '+')
		i++;
	if (!str[i])
		return (0);
	while (str[i])
	{
		if (!ft_isdigit(str[i]))
			return (0);
		i++;
	}
	return (1);
}

/*
** Builtin exit command
** Exits the shell with optional exit code
*/
int	builtin_exit(char **args, t_shell *shell)
{
	int	exit_code;

	write(STDERR_FILENO, "exit\n", 5);
	if (!args[1])
	{
		cleanup_shell(shell);
		exit(shell->last_exit_status);
	}
	if (!is_numeric(args[1]))
	{
		write(STDERR_FILENO, "minishell: exit: ", 17);
		write(STDERR_FILENO, args[1], ft_strlen(args[1]));
		write(STDERR_FILENO, ": numeric argument required\n", 28);
		cleanup_shell(shell);
		exit(255);
	}
	if (args[2])
	{
		write(STDERR_FILENO, "minishell: exit: too many arguments\n", 36);
		return (1);
	}
	exit_code = ft_atoi(args[1]) % 256;
	cleanup_shell(shell);
	exit(exit_code);
}
