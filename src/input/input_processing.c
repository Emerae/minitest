/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_processing.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:40:45 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:40:45 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int	parse_and_validate_input(t_input **head_input, char *line,
		t_shell *shell)
{
	*head_input = cy1_make_list(line);
	if (!*head_input)
		return (1);
	if ((*head_input)->type == 1 && !(*head_input)->next)
	{
		cy0_free_input_list(*head_input);
		return (1);
	}
	if (cy3_substi_check(head_input, shell->env))
	{
		cy0_free_input_list(*head_input);
		shell->last_exit_status = 1;
		return (1);
	}
	if (cy3_scan_dollar_syntax(*head_input, shell->env,
			shell->last_exit_status))
	{
		cy0_free_input_list(*head_input);
		shell->last_exit_status = 1;
		return (1);
	}
	return (0);
}

int	validate_and_convert_syntax(t_input *head_input, t_cmd **head_cmd,
		t_shell *shell)
{
	cy1_remove_space_nodes(&head_input);
	if (validate_syntax(head_input))
	{
		cy0_free_input_list(head_input);
		shell->last_exit_status = ERROR_SYNTAX;
		return (1);
	}
	*head_cmd = cy2_convert_cmd(head_input);
	cy0_free_input_list(head_input);
	if (!*head_cmd)
	{
		shell->last_exit_status = 1;
		return (1);
	}
	return (0);
}

void	process_line(char *line, t_shell *shell)
{
	t_input	*head_input;
	t_cmd	*head_cmd;

	if (parse_and_validate_input(&head_input, line, shell))
		return ;
	if (validate_and_convert_syntax(head_input, &head_cmd, shell))
		return ;
	shell->last_exit_status = execute_command_line(head_cmd, shell);
	cy0_free_cmd_list(head_cmd);
}

void	process_input_line(char *line, t_shell *shell)
{
	if (is_not_only_whitespace(line) == 0)
		add_history(line);
	rl_event_hook = NULL;
	g_signal_received = 0;
	if (!cy0_check_quote_1(line))
		process_line(line, shell);
	rl_event_hook = check_signals_hook;
	if (g_signal_received == SIGINT)
	{
		shell->last_exit_status = 130;
		g_signal_received = 0;
	}
}
