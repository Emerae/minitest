NAME		= minishell
CC			= cc
CFLAGS		= -Wall -Wextra -Werror
INCLUDES	= -I./includes -I./parser
LIBS		= -lreadline

MAIN_SRC	= src/main.c

EXEC_DIR	= src/executor
EXEC_SRC	= $(EXEC_DIR)/executor.c \
			  $(EXEC_DIR)/pipes.c \
			  $(EXEC_DIR)/redirections.c \
			  $(EXEC_DIR)/path.c

BUILTIN_DIR	= src/builtins
BUILTIN_SRC	= $(BUILTIN_DIR)/echo.c \
			  $(BUILTIN_DIR)/cd.c \
			  $(BUILTIN_DIR)/pwd.c \
			  $(BUILTIN_DIR)/export.c \
			  $(BUILTIN_DIR)/unset.c \
			  $(BUILTIN_DIR)/env.c \
			  $(BUILTIN_DIR)/exit.c

SIGNAL_DIR	= src/signals
SIGNAL_SRC	= $(SIGNAL_DIR)/signals.c

UTILS_DIR	= src/utils
UTILS_SRC	= $(UTILS_DIR)/utils.c \
			  $(UTILS_DIR)/utils_str.c \
			  $(UTILS_DIR)/utils_str2.c \
			  $(UTILS_DIR)/env_utils.c \
			  $(UTILS_DIR)/ft_split.c \
			  $(UTILS_DIR)/ft_itoa.c \
			  $(UTILS_DIR)/ft_atoi.c

SRCS		= $(MAIN_SRC) \
			  $(EXEC_SRC) \
			  $(BUILTIN_SRC) \
			  $(SIGNAL_SRC) \
			  $(UTILS_SRC)

PARSER_DIR	= parser
PARS_DIR	= $(PARSER_DIR)/pars
CYUTIL_DIR	= $(PARSER_DIR)/cyutil

PARSER_SRC	= $(PARSER_DIR)/main.c \
			  $(PARS_DIR)/cy0_check_char.c \
			  $(PARS_DIR)/cy0_freeer.c \
			  $(PARS_DIR)/cy0_init_env.c \
			  $(PARS_DIR)/cy1_1_remove_space_nodes.c \
			  $(PARS_DIR)/cy1_env.c \
			  $(PARS_DIR)/cy1_input_list.c \
			  $(PARS_DIR)/cy1_input_list1.c \
			  $(PARS_DIR)/cy1_input_list2.c \
			  $(PARS_DIR)/cy2_1_fill_builtin.c \
			  $(PARS_DIR)/cy2_2_fill_redir.c \
			  $(PARS_DIR)/cy2_2_fill_redir2.c \
			  $(PARS_DIR)/cy2_3_free_first_node.c \
			  $(PARS_DIR)/cy2_convert_cmd.c \
			  $(PARS_DIR)/cy2_convert_cmd2.c \
			  $(PARS_DIR)/cy2_convert_cmd3.c \
			  $(PARS_DIR)/cy3_2_dollar.c \
			  $(PARS_DIR)/cy3_2_dollar2.c \
			  $(PARS_DIR)/cy3_2_dollar_alone.c \
			  $(PARS_DIR)/cy3_2_dollar_bang.c \
			  $(PARS_DIR)/cy3_2_dollar_braces.c \
			  $(PARS_DIR)/cy3_2_dollar_braces2.c \
			  $(PARS_DIR)/cy3_2_dollar_braces3.c \
			  $(PARS_DIR)/cy3_2_dollar_word.c \
			  $(PARS_DIR)/cy3_2_dollar_word2.c \
			  $(PARS_DIR)/cy3_subti_check.c \
			  $(PARS_DIR)/cy3_subti_fuse.c \
			  $(PARS_DIR)/cy4_1wrong_char.c \
			  $(PARS_DIR)/cy4_2wrong_redir.c \
			  $(PARS_DIR)/cy4_3wrong_pipe.c \
			  $(PARS_DIR)/cy4_4wrong_redir_log.c \
			  $(PARS_DIR)/cy4_5wrong_pipe_log.c \
			  $(CYUTIL_DIR)/cy_memset.c \
			  $(CYUTIL_DIR)/cy_strchr.c \
			  $(CYUTIL_DIR)/cy_strcmp.c \
			  $(CYUTIL_DIR)/cy_strdup.c \
			  $(CYUTIL_DIR)/cy_strlcat.c \
			  $(CYUTIL_DIR)/cy_strlcpy.c \
			  $(CYUTIL_DIR)/cy_strlen.c \
			  $(CYUTIL_DIR)/cy_true_strdup.c

PARSER_LIB_SRC = $(filter-out $(PARSER_DIR)/main.c, $(PARSER_SRC))

OBJS		= $(SRCS:.c=.o)
PARSER_OBJS	= $(PARSER_LIB_SRC:.c=.o)
ALL_OBJS	= $(OBJS) $(PARSER_OBJS)

all: $(NAME)

$(NAME): $(ALL_OBJS)
	$(CC) $(CFLAGS) $(ALL_OBJS) -o $(NAME) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(ALL_OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re \
		norm lines test