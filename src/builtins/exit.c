/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:37:44 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:37:44 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

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

static void	handle_exit_no_args(t_shell *shell)
{
	write(STDERR_FILENO, "exit\n", 5);
	shell->should_exit = 1;
}

static int	handle_exit_invalid_arg(char *arg, t_shell *shell)
{
	write(STDERR_FILENO, "minishell: exit: ", 17);
	write(STDERR_FILENO, arg, ft_strlen(arg));
	write(STDERR_FILENO, ": numeric argument required\n", 28);
	shell->last_exit_status = 255;
	shell->should_exit = 1;
	return (0);
}

static int	handle_exit_too_many_args(void)
{
	write(STDERR_FILENO, "minishell: exit: too many arguments\n", 36);
	return (1);
}

int	builtin_exit(char **args, t_shell *shell)
{
	int	exit_code;

	if (!args[1])
	{
		handle_exit_no_args(shell);
		return (0);
	}
	if (!is_numeric(args[1]))
		return (handle_exit_invalid_arg(args[1], shell));
	if (args[2])
		return (handle_exit_too_many_args());
	write(STDERR_FILENO, "exit\n", 5);
	exit_code = ft_atoi(args[1]) % 256;
	shell->last_exit_status = exit_code;
	shell->should_exit = 1;
	return (0);
}
