#ifndef DEBUG_H
# define DEBUG_H

# include <stdio.h>
# include <unistd.h>

/* ************************************************************************** */
/*                           DEBUG CONFIGURATION                              */
/* ************************************************************************** */

/* Main debug switch - set to 1 to enable debug */
# ifndef DEBUG
#  define DEBUG 0
# endif

/* Debug levels */
# define DEBUG_NONE_LEVEL	0
# define DEBUG_ERROR_LEVEL	1
# define DEBUG_INFO_LEVEL	2
# define DEBUG_TRACE_LEVEL	3

/* Current debug level */
# ifndef DEBUG_LEVEL
#  define DEBUG_LEVEL	DEBUG_INFO_LEVEL
# endif

/* Module-specific debug flags */
# ifndef DEBUG_PARSER
#  define DEBUG_PARSER	1
# endif

# ifndef DEBUG_EXEC
#  define DEBUG_EXEC	1
# endif

# ifndef DEBUG_BUILTIN
#  define DEBUG_BUILTIN	1
# endif

# ifndef DEBUG_SIGNAL
#  define DEBUG_SIGNAL	1
# endif

# ifndef DEBUG_REDIR
#  define DEBUG_REDIR	1
# endif

# ifndef DEBUG_PIPE
#  define DEBUG_PIPE	1
# endif

# ifndef DEBUG_ENV
#  define DEBUG_ENV		1
# endif

# ifndef DEBUG_MAIN
#  define DEBUG_MAIN	1
# endif

# ifndef DEBUG_PATH
#  define DEBUG_PATH	1
# endif

/* ************************************************************************** */
/*                              COLOR CODES                                   */
/* ************************************************************************** */

# define COLOR_RESET	"\033[0m"
# define COLOR_RED		"\033[31m"
# define COLOR_GREEN	"\033[32m"
# define COLOR_YELLOW	"\033[33m"
# define COLOR_BLUE		"\033[34m"
# define COLOR_MAGENTA	"\033[35m"
# define COLOR_CYAN		"\033[36m"
# define COLOR_GRAY		"\033[90m"
# define COLOR_WHITE	"\033[97m"

/* ************************************************************************** */
/*                              DEBUG MACROS                                  */
/* ************************************************************************** */

/* Print to stderr with module prefix */
# define DEBUG_PRINT(module, color, fmt, ...) \
	do { \
		if (DEBUG && DEBUG_##module && DEBUG_LEVEL >= DEBUG_INFO_LEVEL) { \
			dprintf(STDERR_FILENO, color "[" #module "] " fmt COLOR_RESET "\n", \
				##__VA_ARGS__); \
		} \
	} while (0)

/* Module-specific debug macros */
# define DEBUG_MAIN_MSG(fmt, ...) \
	DEBUG_PRINT(MAIN, COLOR_WHITE, fmt, ##__VA_ARGS__)

# define DEBUG_PARSER_MSG(fmt, ...) \
	DEBUG_PRINT(PARSER, COLOR_CYAN, fmt, ##__VA_ARGS__)

# define DEBUG_EXEC_MSG(fmt, ...) \
	DEBUG_PRINT(EXEC, COLOR_GREEN, fmt, ##__VA_ARGS__)

# define DEBUG_BUILTIN_MSG(fmt, ...) \
	DEBUG_PRINT(BUILTIN, COLOR_YELLOW, fmt, ##__VA_ARGS__)

# define DEBUG_SIGNAL_MSG(fmt, ...) \
	DEBUG_PRINT(SIGNAL, COLOR_MAGENTA, fmt, ##__VA_ARGS__)

# define DEBUG_REDIR_MSG(fmt, ...) \
	DEBUG_PRINT(REDIR, COLOR_BLUE, fmt, ##__VA_ARGS__)

# define DEBUG_PIPE_MSG(fmt, ...) \
	DEBUG_PRINT(PIPE, COLOR_CYAN, fmt, ##__VA_ARGS__)

# define DEBUG_ENV_MSG(fmt, ...) \
	DEBUG_PRINT(ENV, COLOR_GRAY, fmt, ##__VA_ARGS__)

# define DEBUG_PATH_MSG(fmt, ...) \
	DEBUG_PRINT(PATH, COLOR_GREEN, fmt, ##__VA_ARGS__)

/* Error messages (always shown if DEBUG is on) */
# define DEBUG_ERROR(fmt, ...) \
	do { \
		if (DEBUG && DEBUG_LEVEL >= DEBUG_ERROR_LEVEL) { \
			dprintf(STDERR_FILENO, COLOR_RED "[ERROR] " fmt COLOR_RESET "\n", \
				##__VA_ARGS__); \
		} \
	} while (0)

/* Trace messages (only shown at highest debug level) */
# define DEBUG_TRACE(fmt, ...) \
	do { \
		if (DEBUG && DEBUG_LEVEL >= DEBUG_TRACE_LEVEL) { \
			dprintf(STDERR_FILENO, COLOR_GRAY "[TRACE] %s:%d: " fmt COLOR_RESET "\n", \
				__FILE__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

/* ************************************************************************** */
/*                          HELPER DEBUG FUNCTIONS                            */
/* ************************************************************************** */

/* Print command structure */
static inline void	debug_print_cmd(t_cmd *cmd)
{
	int		i;
	t_redir	*redir;

	if (!DEBUG || !DEBUG_EXEC || DEBUG_LEVEL < DEBUG_INFO_LEVEL)		return ;
	dprintf(STDERR_FILENO, COLOR_GREEN "[EXEC] Command: " COLOR_RESET);
	if (cmd->args)
	{
		i = 0;
		while (cmd->args[i])
		{
			dprintf(STDERR_FILENO, "'%s' ", cmd->args[i]);
			i++;
		}
	}
	else
		dprintf(STDERR_FILENO, "(no args)");
	dprintf(STDERR_FILENO, "\n");
	if (cmd->builtin_id != -1)
		dprintf(STDERR_FILENO, COLOR_GREEN "[EXEC] Builtin ID: %d\n" COLOR_RESET,
			cmd->builtin_id);
	redir = cmd->redirs;
	while (redir)
	{
		dprintf(STDERR_FILENO, COLOR_GREEN "[EXEC] Redirection: type=%d, file='%s'\n"
			COLOR_RESET, redir->type, redir->file);
		redir = redir->next;
	}
}

/* Print environment variable */
static inline void	debug_print_env_var(const char *key, const char *value)
{
	if (!DEBUG || !DEBUG_ENV || DEBUG_LEVEL < DEBUG_INFO_LEVEL)
		return ;
	if (value)
		dprintf(STDERR_FILENO, COLOR_GRAY "[ENV] %s=%s\n" COLOR_RESET, key, value);
	else
		dprintf(STDERR_FILENO, COLOR_GRAY "[ENV] %s=(null)\n" COLOR_RESET, key);
}

/* Print pipe state */
static inline void	debug_print_pipe_state(int prev_pipe, int pipe_fd[2])
{
	if (!DEBUG || !DEBUG_PIPE || DEBUG_LEVEL < DEBUG_INFO_LEVEL)
		return ;
	if (pipe_fd)
		dprintf(STDERR_FILENO, COLOR_CYAN "[PIPE] prev_pipe=%d, pipe[0]=%d, pipe[1]=%d\n"
			COLOR_RESET, prev_pipe, pipe_fd[0], pipe_fd[1]);
	else
		dprintf(STDERR_FILENO, COLOR_CYAN "[PIPE] prev_pipe=%d, pipe=(null)\n"
			COLOR_RESET, prev_pipe);
}

#endif