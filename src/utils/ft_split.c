#include "../../includes/minishell.h"

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

static int	get_word_len(char const *s, char c)
{
	int	len;

	len = 0;
	while (s[len] && s[len] != c)
		len++;
	return (len);
}

static void	free_split(char **array, int i)
{
	while (i > 0)
	{
		i--;
		free(array[i]);
	}
	free(array);
}

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

static char	**allocate_split_array(char const *s, char c)
{
	char	**array;
	int		words;

	if (!s)
		return (NULL);
	words = count_words(s, c);
	array = malloc(sizeof(char *) * (words + 1));
	return (array);
}

static int	fill_split_array(char **array, char const *s, char c)
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
