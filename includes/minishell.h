/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:43:23 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:43:23 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MINISHELL_H
# define MINISHELL_H

# include <stdlib.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <signal.h>
# include <fcntl.h>
# include <errno.h>
# include <readline/readline.h>
# include <readline/history.h>
# include "../parser/prser.h"
# define PROMPT "minishell$ "
# define ERROR_CMD_NOT_FOUND 127
# define ERROR_PERMISSION 126
# define ERROR_SYNTAX 2
# define ERROR_MALLOC 1

extern volatile sig_atomic_t	g_signal_received;

typedef struct s_shell
{
	char	**env;
	int		last_exit_status;
	int		in_pipe;
	int		in_child;
	pid_t	current_child_pid;
	int		should_exit;
}			t_shell;

typedef struct s_exec
{
	int		pipe_fd[2];
	int		prev_pipe;
	pid_t	pid;
	int		status;
}			t_exec;

typedef struct s_pipeline_data
{
	pid_t	*pids;
	int		index;
}	t_pipeline_data;

/* main.c */
void			shell_loop(t_shell *shell);
void			init_shell(t_shell *shell, char **envp);
void			cleanup_shell(t_shell *shell);

/* input/input_validation.c */
int				is_not_only_whitespace(const char *s);
int				validate_syntax(t_input *head_input);
void			handle_signal_interrupt(t_shell *shell, char *line);

/* input/input_processing.c */
int				parse_and_validate_input(t_input **head_input, char *line,
					t_shell *shell);
int				validate_and_convert_syntax(t_input *head_input,
					t_cmd **head_cmd, t_shell *shell);
void			process_line(char *line, t_shell *shell);
void			process_input_line(char *line, t_shell *shell);

/* executor/executor.c */
int				execute_simple_command(t_cmd *cmd, t_shell *shell);
int				execute_command_line(t_cmd *cmd_list, t_shell *shell);

/* executor/executor_builtins.c */
int				is_builtin(char *cmd);
int				execute_builtin(t_cmd *cmd, t_shell *shell);
int				must_run_in_parent(char *cmd);
int				execute_builtin_with_redirs(t_cmd *cmd, t_shell *shell);

/* executor/pipes_basic.c */
int				create_pipe(int pipe_fd[2]);
void			setup_pipe_child(int pipe_fd[2], int prev_pipe, int is_last);
void			close_pipe_parent(int pipe_fd[2], int *prev_pipe);

/* executor/heredoc.c */
int				preprocess_heredocs(t_cmd *cmd_list);

/* executor/command_execution.c */
int				execute_and_manage_pipes(t_cmd *current, t_shell *shell,
					t_exec *exec, t_pipeline_data *data);

/* executor/pipeline.c */
int				execute_pipeline(t_cmd *cmd_list, t_shell *shell);

/* executor/redirections.c */
int				setup_redirections(t_redir *redirs);
int				open_file_for_redirect(char *filename, int type);
int				handle_heredoc(char *delimiter);

/* executor/redirection_utils.c */
int				handle_heredoc_interruption(int pipe_fd[2]);
int				process_heredoc_line(char *line, char *delimiter,
					int pipe_fd[2]);
int				read_heredoc_loop(char *delimiter, int pipe_fd[2]);

/* executor/path.c */
char			*find_command_path(char *cmd, char **env);
char			**get_paths_from_env(char **env);
char			*check_path(char *dir, char *cmd);

/* executor/path_search.c */
char			*search_in_path(char *cmd, char **env);

/* executor/path_expansion.c */
char			**expand_args_tildes(char **args, char **env);
char			*expand_single_tilde(char *arg, char **env);

/* builtins/echo.c */
int				builtin_echo(char **args);

/* builtins/cd.c */
int				builtin_cd(char **args, char ***env);

/* builtins/cd_path.c */
char			*normalize_path_segments(char *path);
char			*normalize_path(char *path, char **env);

/* builtins/cd_utils.c */
int				calculate_total_length(char **stack, int count);
void			build_path_string(char **stack, int count,
					char *result, int absolute);
char			*rebuild_path(char **stack, int count, int absolute);

/* builtins/pwd.c */
int				builtin_pwd(void);

/* builtins/export.c */
int				builtin_export(char **args, char ***env);

/* builtins/export_display.c */
void			print_sorted_env(char **env);
void			print_export_error(char *arg);

/* builtins/unset.c */
int				builtin_unset(char **args, char ***env);

/* builtins/env.c */
int				builtin_env(char **env);

/* builtins/exit.c */
int				builtin_exit(char **args, t_shell *shell);

/* signals/signals.c */
void			handle_sigusr1(int sig);
void			setup_signals(void);
void			setup_child_signals(void);
void			handle_sigint(int sig);
void			handle_sigquit(int sig);
int				check_signals_hook(void);

/* signals/signal_heredoc.c */
void			handle_sigint_heredoc(int sig);
void			setup_heredoc_signals(void);

/* environment/env_core.c */
char			**copy_env(char **envp);
void			free_env(char **env);
char			*get_env_value(char **env, char *key);
char			**realloc_env(char **env, int new_size);
int				find_env_index(char **env, char *key);

/* environment/env_modify.c */
int				set_env_value(char ***env, char *key, char *value);
int				unset_env_value(char ***env, char *key);

/* memory/array_utils.c */
void			free_string_array(char **array);
int				count_string_array(char **array);
char			**duplicate_string_array(char **array);

/* memory/cleanup.c */
void			error_exit(char *msg, int exit_code);
void			print_error(char *cmd, char *msg);

/* string/string_basic.c */
int				ft_strcmp(const char *s1, const char *s2);
int				ft_strncmp(const char *s1, const char *s2, size_t n);
size_t			ft_strlen(const char *s);
char			*ft_strchr(const char *s, int c);
int				ft_isdigit(int c);

/* string/string_copy.c */
char			*ft_strdup(const char *s);
char			*ft_strjoin(char const *s1, char const *s2);
char			*ft_strcpy(char *dst, const char *src);
char			*ft_strcat(char *dst, const char *src);
char			*ft_strncpy(char *dst, const char *src, size_t n);

/* string/ft_strstr.c */
char			*ft_strstr(const char *haystack, const char *needle);

/* string/ft_atoi.c */
int				ft_atoi(const char *str);

/* string/ft_itoa.c */
char			*ft_itoa(int n);

/* string/ft_split_utils.c */
int				count_words(char const *s, char c);
int				get_word_len(char const *s, char c);
void			free_split(char **array, int i);
char			*extract_word(char const *s, int len);
char			**allocate_split_array(char const *s, char c);

/* string/ft_split.c */
int				fill_split_array(char **array, char const *s, char c);
char			**ft_split(char const *s, char c);

#endif