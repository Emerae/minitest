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

static    int is_space(char c)
{
    if (c == ' ' || c == '\t' || c == '\v' || c == '\n' || c == '\f' || c == '\r')
        return (1);
    return (0);
}

static int is_not_only_whitespace(const char *s)
{
    int i;

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

/*
void	print_input_list(t_input *head_input)
{
	t_input *current;

	current = head_input;
	printf("type 1 space, 2 text, 3 '', 4 \"\"\n");
	while (current)
	{
		if (current->input)
		{
			printf("[%d]inp _%s_\n", current->number, current->input);
			if (current->input_num)
				printf("[%d]num _%s_\n", current->number, current->input_num);
			if (current->input_type)
				printf("[%d]typ _%s_\n", current->number, current->input_type);
			printf("[%d] type: %d\n", current->number, current->type);
		}
		else
			printf("[%d] (null)\n", current->number);
		printf("\n");
		current = current->next;
	}
}

void	print_envv(char **env)
{
	int	i = 0;

	while (env[i])
	{
		printf("l%d:%s\n",i,env[i]);
		i = i + 1;
	}
	printf("----\n");
}

void	print_cmd_list(t_cmd *head_cmd)
{
	t_cmd *cmd = head_cmd;
	int arg_index;
	t_redir *redir;

	while (cmd)
	{
		printf("Command:\n");

		// Print args
		if (cmd->args)
		{
			arg_index = 0;
			while (cmd->args[arg_index])
			{
				printf("  arg %d : _%s_\n", arg_index + 1, cmd->args[arg_index]);
				arg_index = arg_index + 1;
			}
		}
		else
			printf("  No arguments\n");

		// Print builtin ID
		if (cmd->builtin_id == -1)
			printf("  builtin : not a builtin\n");
		else if (cmd->builtin_id == 1)
			printf("  builtin : echo -n\n");
		else if (cmd->builtin_id == 2)
			printf("  builtin : echo\n");
		else if (cmd->builtin_id == 3)
			printf("  builtin : cd\n");
		else if (cmd->builtin_id == 4)
			printf("  builtin : pwd\n");
		else if (cmd->builtin_id == 5)
			printf("  builtin : export\n");
		else if (cmd->builtin_id == 6)
			printf("  builtin : unset\n");
		else if (cmd->builtin_id == 7)
			printf("  builtin : env\n");
		else if (cmd->builtin_id == 8)
			printf("  builtin : exit\n");
		else
			printf("  builtin : bug %d\n", cmd->builtin_id);

		// Print redirections
		if (cmd->redirs)
		{
			printf("  redirections:\n");
			redir = cmd->redirs;
			while (redir)
			{
				if (redir->type == 0)
					printf("    Type: <  File: %s\n", redir->file);
				else if (redir->type == 1)
					printf("    Type: >  File: %s\n", redir->file);
				else if (redir->type == 2)
					printf("    Type: >> File: %s\n", redir->file);
				else if (redir->type == 3)
					printf("    Type: << File: %s\n", redir->file);
				else
				printf("Problem Type: %d  File: %s\n", redir->type, redir->file);
				redir = redir->next;
			}
		}
		else
			printf("  No redirections\n");
		// Move to the next command in the pipeline
		cmd = cmd->next;
		if (cmd)
			printf("---- next command ----\n");
	}
}
*/
