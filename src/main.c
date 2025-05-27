#include "../includes/minishell.h"

volatile sig_atomic_t	g_signal_received = 0;

/*
** Initialize shell structure
*/
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
}

/*
** Clean up shell resources
*/
void	cleanup_shell(t_shell *shell)
{
	if (shell->env)
		free_env(shell->env);
}

/*
** Process a command line
*/
static void	process_line(char *line, t_shell *shell)
{
	t_input	*head_input;
	t_cmd	*head_cmd;
	
	head_input = cy1_make_list(line);
	if (!head_input)
		return ;
	if (head_input->type == 1 && !head_input->next)
	{
		cy0_free_input_list(head_input);
		return ;
	}
	if (cy3_substi_check(&head_input, shell->env))
	{
		cy0_free_input_list(head_input);
		shell->last_exit_status = 1;
		return ;
	}
	if (cy3_scan_dollar_syntax(head_input, shell->env, shell->last_exit_status))
	{
		cy0_free_input_list(head_input);
		shell->last_exit_status = 1;
		return ;
	}
	cy1_remove_space_nodes(&head_input);
	if (cy4_1wrong_char(head_input) || cy4_2wrong_redir(head_input)
		|| cy4_3wrong_pipe(head_input) || cy4_4wrong_redir_log(head_input)
		|| cy4_5wrong_pipe_log(head_input))
	{
		write(STDERR_FILENO, "minishell: syntax error\n", 24);
		cy0_free_input_list(head_input);
		shell->last_exit_status = ERROR_SYNTAX;
		return ;
	}
	head_cmd = cy2_convert_cmd(head_input);
	cy0_free_input_list(head_input);
	if (!head_cmd)
	{
		shell->last_exit_status = 1;
		return ;
	}
	shell->last_exit_status = execute_command_line(head_cmd, shell);
	cy0_free_cmd_list(head_cmd);
}

void	shell_loop(t_shell *shell)
{
	char	*line;

	while (1)
	{
		line = readline(PROMPT);
		
		if (g_signal_received == SIGINT)
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
			continue;
		}
		if (!line)
		{
			write(STDOUT_FILENO, "exit\n", 5);
			break ;
		}
		if (*line)
		{
			add_history(line);
			rl_event_hook = NULL;
			g_signal_received = 0;
			process_line(line, shell);
			rl_event_hook = check_signals_hook;
			if (g_signal_received == SIGINT)
			{
				shell->last_exit_status = 130;
				g_signal_received = 0;
			}
		}
		free(line);
	}
}

/*
** Main function
*/
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
