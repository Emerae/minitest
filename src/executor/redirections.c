/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redirections.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:40:38 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:40:38 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	open_file_for_redirect(char *filename, int type)
{
	int	fd;

	if (type == 0)
		fd = open(filename, O_RDONLY);
	else if (type == 1)
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (type == 2)
		fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		return (-1);
	if (fd == -1)
	{
		write(STDERR_FILENO, "minishell: ", 11);
		write(STDERR_FILENO, filename, ft_strlen(filename));
		write(STDERR_FILENO, ": ", 2);
		perror("");
	}
	return (fd);
}

int	handle_heredoc(char *delimiter)
{
	int	pipe_fd[2];

	if (pipe(pipe_fd) == -1)
		return (-1);
	setup_heredoc_signals();
	if (read_heredoc_loop(delimiter, pipe_fd) == -1)
		return (-1);
	close(pipe_fd[1]);
	setup_signals();
	return (pipe_fd[0]);
}

static int	apply_redirection(t_redir *redir)
{
	int	fd;
	int	target_fd;

	if (redir->type == 3)
		fd = handle_heredoc(redir->file);
	else
		fd = open_file_for_redirect(redir->file, redir->type);
	if (fd == -1)
		return (-1);
	if (redir->type == 0 || redir->type == 3)
		target_fd = STDIN_FILENO;
	else
		target_fd = STDOUT_FILENO;
	if (dup2(fd, target_fd) == -1)
	{
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

int	setup_redirections(t_redir *redirs)
{
	t_redir	*current;

	current = redirs;
	while (current)
	{
		if (apply_redirection(current) == -1)
			return (-1);
		current = current->next;
	}
	return (0);
}
