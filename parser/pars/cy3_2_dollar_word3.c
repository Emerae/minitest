#include "../prser.h"

void	cy3_handle_dollar_word_key(t_input *current, t_dollar_word *s)
{
	s->keylen = 0;
	s->k = s->i + 1;
	while (s->k <= s->j && s->keylen < 255)
	{
		s->key[s->keylen] = current->input[s->k];
		s->k = s->k + 1;
		s->keylen = s->keylen + 1;
	}
	s->key[s->keylen] = '\0';
}

void	cy3_handle_dollar_word_findenv(t_dollar_word *s, char **env, int *flag)
{
	char	*equal;

	s->e = 0;
	while (env[s->e])
	{
		equal = cy_strchr(env[s->e], '=');
		if (!equal)
		{
			s->e = s->e + 1;
			continue ;
		}
		if ((int)(equal - env[s->e]) == s->keylen &&
			cy_strncmp(env[s->e], s->key, s->keylen) == 0)
		{
			*flag = s->e;
			break ;
		}
		s->e = s->e + 1;
	}
}
