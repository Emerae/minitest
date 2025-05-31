#include "../prser.h"

int	cy3_handle_dollar_word_3a(t_input *current, int i, t_dollar_word *s)
{
	s->p = 0;
	s->k = 0;
	while (s->k < i)
	{
		s->new_input[s->p] = current->input[s->k];
		s->new_type[s->p] = current->input_type[s->k];
		s->new_num[s->p] = current->input_num[s->k];
		s->p = s->p + 1;
		s->k = s->k + 1;
	}
	return (0);
}

int	cy3_handle_dollar_word_3b(t_input *current, int j, t_dollar_word *s)
{
	s->k = j + 1;
	while (current->input[s->k])
	{
		s->new_input[s->p] = current->input[s->k];
		s->new_type[s->p] = current->input_type[s->k];
		s->new_num[s->p] = current->input_num[s->k];
		s->p = s->p + 1;
		s->k = s->k + 1;
	}
	return (0);
}

int	cy3_handle_dollar_word_3(t_input *current, int i, int j)
{
	t_dollar_word	s;

	s.lold = cy_strlen(current->input);
	s.new_input = malloc(s.lold - (j - i + 1) + 1);
	s.new_type = malloc(s.lold - (j - i + 1) + 1);
	s.new_num = malloc(s.lold - (j - i + 1) + 1);
	if (!s.new_input || !s.new_type || !s.new_num)
		return (-1);
	cy3_handle_dollar_word_3a(current, i, &s);
	cy3_handle_dollar_word_3b(current, j, &s);
	s.new_input[s.p] = '\0';
	s.new_type[s.p] = '\0';
	s.new_num[s.p] = '\0';
	free(current->input);
	free(current->input_type);
	free(current->input_num);
	current->input = s.new_input;
	current->input_type = s.new_type;
	current->input_num = s.new_num;
	return (i);
}

int    cy3_handle_dollar_word_1(t_input *current, char **env, t_dollar_word *s, int flag)
{
    //char    *equal;

    if (flag >= 0)
    {
        s->value = cy_strchr(env[s->e], '=') + 1;
        return (cy3_handle_dollar_word_2(current, s));
    }
    return (cy3_handle_dollar_word_3(current, s->i, s->j));
}

/*
** CORRECTION CRITIQUE - Suppression du code dupliqué dangereux
**
** Problème : cy3_handle_dollar_word_1 essayait de copier des données dans
** s->new_input AVANT que la mémoire soit allouée ! 
** 
** Valgrind montre : "Invalid write at address 0x0" car s->new_input = NULL
**
** Solution : Supprimer ce code dupliqué et laisser cy3_handle_dollar_word_2
** s'occuper de TOUTE l'allocation et l'assemblage de la chaîne.

int	cy3_handle_dollar_word_1(t_input *current, char **env, t_dollar_word *s)
{
	char	*equal;

	if (env[s->e])
	{
		equal = cy_strchr(env[s->e], '=');
		if (equal && (int)(equal - env[s->e]) == s->keylen &&
			cy_strncmp(env[s->e], s->key, s->keylen) == 0)
		{
			s->value = equal + 1;
			return (cy3_handle_dollar_word_2(current, s));
		}
	}
	return (cy3_handle_dollar_word_3(current, s->i, s->j));
}
*/

/*
** CORRECTION CRITIQUE - Bug de vérification de variable trouvée
**
** Problème 2 : La condition if (env[s->e + 1]) vérifie si l'élément SUIVANT 
** existe, pas si notre variable a été trouvée !
**
** Exemple : Si TEST_VAR est le dernier élément de env :
** - findenv trouve TEST_VAR à l'index s->e
** - mais env[s->e + 1] est NULL  
** - donc on appelle cy3_handle_dollar_word_3 qui SUPPRIME $TEST_VAR
** - au lieu de cy3_handle_dollar_word_2 qui le remplacerait par "hello"
**
** Résultat : echo $TEST_VAR affiche une ligne vide au lieu de "hello"
**
** Solution : Vérifier si env[s->e] existe ET contient bien notre variable
*/
/*
int	cy3_handle_dollar_word_1(t_input *current, char **env, t_dollar_word *s)
{
	char	*equal;

	if (env[s->e])
	{
		equal = cy_strchr(env[s->e], '=');
		if (equal && (int)(equal - env[s->e]) == s->keylen &&
			cy_strncmp(env[s->e], s->key, s->keylen) == 0)
		{
			s->value = equal + 1;
			s->p = 0;
			s->k = 0;
			while (s->k < s->i)
			{
				s->new_input[s->p] = current->input[s->k];
				s->new_type[s->p] = current->input_type[s->k];
				s->new_num[s->p] = current->input_num[s->k];
				s->p = s->p + 1;
				s->k = s->k + 1;
			}
			return (cy3_handle_dollar_word_2(current, s));
		}
	}
	return (cy3_handle_dollar_word_3(current, s->i, s->j));
}
*/

/*
int	cy3_handle_dollar_word_1(t_input *current, char **env, t_dollar_word *s)
{
	if (env[s->e + 1])
	{
		s->value = cy_strchr(env[s->e], '=') + 1;
		s->p = 0;
		s->k = 0;
		while (s->k < s->i)
		{
			s->new_input[s->p] = current->input[s->k];
			s->new_type[s->p] = current->input_type[s->k];
			s->new_num[s->p] = current->input_num[s->k];
			s->p = s->p + 1;
			s->k = s->k + 1;
		}
		return (cy3_handle_dollar_word_2(current, s));
	}
	return (cy3_handle_dollar_word_3(current, s->i, s->j));
}
*/

int    cy3_handle_dollar_word(t_input *current, int i, int j, char **env)
{
    t_dollar_word    s;
    int                flag;

    s.i = i;
    s.j = j;
    flag = -1;
    cy3_handle_dollar_word_key(current, &s);
    cy3_handle_dollar_word_findenv(&s, env, &flag);
    return (cy3_handle_dollar_word_1(current, env, &s, flag));
}

/*
int	cy3_handle_dollar_word(t_input *current, int i, int j, char **env)
{
	t_dollar_word	s;

	s.i = i;
	s.j = j;
	cy3_handle_dollar_word_key(current, &s);
	cy3_handle_dollar_word_findenv(&s, env);
	return (cy3_handle_dollar_word_1(current, env, &s));
}
*/

