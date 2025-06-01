/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_validation.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:40:54 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:40:54 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int	is_space(char c)
{
	if (c == ' ' || c == '\t' || c == '\v' || c == '\n'
		|| c == '\f' || c == '\r')
		return (1);
	return (0);
}

int	is_not_only_whitespace(const char *s)
{
	int	i;

	if (!s)
		return (1);
	i = 0;
	while (s[i])
	{
		if (is_space(s[i]) == 0)
			return (0);
		i = i + 1;
	}
	return (1);
}

int	validate_syntax(t_input *head_input)
{
	if (cy4_1wrong_char(head_input) || cy4_2wrong_redir(head_input)
		|| cy4_3wrong_pipe(head_input)
		|| cy4_4wrong_redir_log(head_input)
		|| cy4_5wrong_pipe_log(head_input))
	{
		write(STDERR_FILENO, "minishell: syntax error\n", 24);
		return (1);
	}
	return (0);
}

void	handle_signal_interrupt(t_shell *shell, char *line)
{
	shell->last_exit_status = 130;
	g_signal_received = 0;
	if (shell->current_child_pid > 0)
	{
		kill(shell->current_child_pid, SIGINT);
		shell->current_child_pid = 0;
	}
	if (line)
		free(line);
}
