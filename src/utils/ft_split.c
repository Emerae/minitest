#include "../../includes/minishell.h"

/*
** Count number of words separated by delimiter
*/
static int	count_words(char const *s, char c)
{
	int	count;
	int	in_word;

	count = 0;
	in_word = 0;
	while (*s)
	{
		if (*s != c && !in_word)
		{
			in_word = 1;
			count++;
		}
		else if (*s == c)
			in_word = 0;
		s++;
	}
	return (count);
}

/*
** Get length of next word
*/
static int	get_word_len(char const *s, char c)
{
	int	len;

	len = 0;
	while (s[len] && s[len] != c)
		len++;
	return (len);
}

/*
** Free array on allocation error
*/
static void	free_split(char **array, int i)
{
	while (i > 0)
	{
		i--;
		free(array[i]);
	}
	free(array);
}

/*
** Extract word from string
*/
static char	*extract_word(char const *s, int len)
{
	char	*word;
	int		i;

	word = malloc(len + 1);
	if (!word)
		return (NULL);
	i = 0;
	while (i < len)
	{
		word[i] = s[i];
		i++;
	}
	word[i] = '\0';
	return (word);
}

/*
** Split string by delimiter
*/
char	**ft_split(char const *s, char c)
{
	char	**array;
	int		words;
	int		i;
	int		len;

	if (!s)
		return (NULL);
	words = count_words(s, c);
	array = malloc(sizeof(char *) * (words + 1));
	if (!array)
		return (NULL);
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
			return (NULL);
		}
		s += len;
		i++;
	}
	array[i] = NULL;
	return (array);
}
