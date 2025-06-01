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
}

void	cleanup_shell(t_shell *shell)
{
	if (shell->env)
		free_env(shell->env);
}

static int	is_space(char c)
{
	if (c == ' ' || c == '\t' || c == '\v' || c == '\n'
		|| c == '\f' || c == '\r')
		return (1);
	return (0);
}

static int	is_not_only_whitespace(const char *s)
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

static int	validate_syntax(t_input *head_input)
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

static int	parse_and_validate_input(t_input **head_input, char *line,
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

static int	validate_and_convert_syntax(t_input *head_input, t_cmd **head_cmd,
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

static void	process_line(char *line, t_shell *shell)
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

static void	handle_signal_interrupt(t_shell *shell, char *line)
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

static void	process_input_line(char *line, t_shell *shell)
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
