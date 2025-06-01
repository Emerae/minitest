/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:42:20 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:42:20 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

volatile sig_atomic_t	g_signal_received = 0;

void	init_shell(t_shell *shell, char **envp)
{
	shell->env = copy_env(envp);
	if (!shell->env)
	{
		write(STDERR_FILENO, "minishell: failed to copy environment\n", 38);
		exit(1);
	}
	shell->last_exit_status = 0;
	shell->in_pipe = 0;
	shell->in_child = 0;
	shell->current_child_pid = 0;
	shell->should_exit = 0;
}

void	cleanup_shell(t_shell *shell)
{
	if (shell->env)
		free_env(shell->env);
}

void	shell_loop(t_shell *shell)
{
	char	*line;

	while (1)
	{
		line = readline(PROMPT);
		if (g_signal_received == SIGINT)
		{
			handle_signal_interrupt(shell, line);
			continue ;
		}
		if (!line)
		{
			write(STDOUT_FILENO, "exit\n", 5);
			break ;
		}
		if (*line)
			process_input_line(line, shell);
		free(line);
	}
}

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;

	(void)argc;
	(void)argv;
	init_shell(&shell, envp);
	setup_signals();
	shell_loop(&shell);
	cleanup_shell(&shell);
	rl_clear_history();
	return (shell.last_exit_status);
}
