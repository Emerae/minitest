/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlaigle <rlaigle@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/01 22:41:52 by rlaigle           #+#    #+#             */
/*   Updated: 2025/06/01 22:41:52 by rlaigle          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	fill_split_array(char **array, char const *s, char c)
{
	int		words;
	int		i;
	int		len;

	words = count_words(s, c);
	i = 0;
	while (i < words)
	{
		while (*s && *s == c)
			s++;
		len = get_word_len(s, c);
		array[i] = extract_word(s, len);
		if (!array[i])
		{
			free_split(array, i);
			return (1);
		}
		s += len;
		i++;
	}
	array[i] = NULL;
	return (0);
}

char	**ft_split(char const *s, char c)
{
	char	**array;

	array = allocate_split_array(s, c);
	if (!array)
		return (NULL);
	if (fill_split_array(array, s, c))
		return (NULL);
	return (array);
}
