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

/* ************************************************************************** */
/*                                   MACROS                                   */
/* ************************************************************************** */

# define PROMPT "minishell$ "
# define ERROR_CMD_NOT_FOUND 127
# define ERROR_PERMISSION 126
# define ERROR_SYNTAX 2
# define ERROR_MALLOC 1

/* ************************************************************************** */
/*                              GLOBAL VARIABLE                               */
/* ************************************************************************** */

extern volatile sig_atomic_t	g_signal_received;

/* ************************************************************************** */
/*                                STRUCTURES                                  */
/* ************************************************************************** */

typedef struct s_shell
{
	char	**env;
	int		last_exit_status;
	int		in_pipe;
	int		in_child;
	pid_t	current_child_pid;  /* Ajoutez ça */
}	t_shell;

typedef struct s_exec
{
	int		pipe_fd[2];
	int		prev_pipe;
	pid_t	pid;
	int		status;
}	t_exec;

/* ************************************************************************** */
/*                               MAIN FUNCTIONS                               */
/* ************************************************************************** */

/* main.c */
void	shell_loop(t_shell *shell);
void	init_shell(t_shell *shell, char **envp);
void	cleanup_shell(t_shell *shell);

/* ************************************************************************** */
/*                                 EXECUTOR                                   */
/* ************************************************************************** */

/* executor.c */
int		execute_command_line(t_cmd *cmd_list, t_shell *shell);
int		execute_simple_command(t_cmd *cmd, t_shell *shell);
int		execute_pipeline(t_cmd *cmd_list, t_shell *shell);

/* pipes.c */
int		create_pipe(int pipe_fd[2]);
void	setup_pipe_child(int pipe_fd[2], int prev_pipe, int is_last);
void	close_pipe_parent(int pipe_fd[2], int *prev_pipe);

/* redirections.c */
int		setup_redirections(t_redir *redirs);
int		open_file_for_redirect(char *filename, int type);
int		handle_heredoc(char *delimiter);

/* path.c */
char	*find_command_path(char *cmd, char **env);
char	**get_paths_from_env(char **env);
int		is_builtin(char *cmd);

/* ************************************************************************** */
/*                                 BUILTINS                                   */
/* ************************************************************************** */

/* builtins_manager.c */
int		execute_builtin(t_cmd *cmd, t_shell *shell);

/* echo.c */
int		builtin_echo(char **args);

/* cd.c */
int		builtin_cd(char **args, char ***env);

/* pwd.c */
int		builtin_pwd(void);

/* export.c */
int		builtin_export(char **args, char ***env);

/* unset.c */
int		builtin_unset(char **args, char ***env);

/* env.c */
int		builtin_env(char **env);

/* exit.c */
int		builtin_exit(char **args, t_shell *shell);

/* ************************************************************************** */
/*                                  SIGNALS                                   */
/* ************************************************************************** */

/* signals.c */
void	handle_sigusr1(int sig);
void	setup_signals(void);
void	setup_child_signals(void);
void	handle_sigint(int sig);
void	handle_sigquit(int sig);
void	handle_sigint_heredoc(int sig);
void	setup_heredoc_signals(void);
int		check_signals_hook(void);

/* signal_utils.c */
void	setup_terminal(t_shell *shell);
void	restore_terminal_attrs(t_shell *shell);
void	handle_sigint_heredoc(int sig);
void	setup_heredoc_signals(void);

/* ************************************************************************** */
/*                              ENVIRONMENT                                   */
/* ************************************************************************** */

/* env_utils.c */
char	**copy_env(char **envp);
void	free_env(char **env);
char	*get_env_value(char **env, char *key);
int		set_env_value(char ***env, char *key, char *value);
int		unset_env_value(char ***env, char *key);

/* ************************************************************************** */
/*                                  UTILS                                     */
/* ************************************************************************** */

/* utils.c */
void	free_string_array(char **array);
int		count_string_array(char **array);
char	**duplicate_string_array(char **array);
void	error_exit(char *msg, int exit_code);
void	print_error(char *cmd, char *msg);

/* utils2.c */
int		ft_strcmp(const char *s1, const char *s2);
int		ft_strncmp(const char *s1, const char *s2, size_t n);
char	*ft_strdup(const char *s);
size_t	ft_strlen(const char *s);
char	*ft_strjoin(char const *s1, char const *s2);

/* utils3.c */
char	**ft_split(char const *s, char c);
char	*ft_strchr(const char *s, int c);
int		ft_isdigit(int c);
int		ft_atoi(const char *str);
char	*ft_itoa(int n);

/* utils_str.c */
char	*ft_strcpy(char *dst, const char *src);
char	*ft_strcat(char *dst, const char *src);

/* utils_str2.c */
char	*ft_strncpy(char *dst, const char *src, size_t n);

#endif