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

/*
** CORRECTION CRITIQUE - Bug d'expansion de variables
** 
** Problème 1 : La boucle while (env[s->e + 1]) ne vérifie JAMAIS le dernier 
** élément de l'environnement ! 
** 
** Exemple concret : Si env = ["PATH=...", "HOME=...", "TEST_VAR=hello", NULL]
** et qu'on cherche TEST_VAR (index 2), la boucle vérifie :
** - env[0] car env[1] existe
** - env[1] car env[2] existe  
** - STOP car env[3] est NULL
** -> TEST_VAR n'est jamais vérifié !
**
** Conséquence : Toute variable en fin d'environnement est "non trouvée"
** et supprimée au lieu d'être remplacée par sa valeur.
**
** Solution : Vérifier env[s->e] directement au lieu de env[s->e + 1]
*/
void	cy3_handle_dollar_word_findenv(t_dollar_word *s, char **env)
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
			break ;
		s->e = s->e + 1;
	}
}

/*
void	cy3_handle_dollar_word_findenv(t_dollar_word *s, char **env)
{
	char	*equal;

	s->e = 0;
	while (env[s->e + 1])
	{
		equal = cy_strchr(env[s->e], '=');
		if (!equal)
		{
			s->e = s->e + 1;
			continue ;
		}
		if ((int)(equal - env[s->e]) == s->keylen &&
			cy_strncmp(env[s->e], s->key, s->keylen) == 0)
			break ;
		s->e = s->e + 1;
	}
}
*/

/*
** CORRECTION CRITIQUE - Copie manquante de la partie avant la variable
**
** Problème : cy3_handle_dollar_word_2 ne copie que la valeur de la variable
** et ce qui vient après, mais JAMAIS ce qui vient avant.
**
** Exemple : "User: '$USER' Home: $HOME"
** - Pour $HOME, on doit copier "User: 'rlaigle' Home: " + "/home/rlaigle"
** - Actuellement : on copie seulement "/home/rlaigle"
**
** Solution : Ajouter la copie de current->input[0..s->i-1] avant la variable
*/
static int	cy3_handle_dollar_word_2_before(t_input *current, t_dollar_word *s)
{
	s->k = 0;
	while (s->k < s->i)
	{
		s->new_input[s->p] = current->input[s->k];
		s->new_type[s->p] = current->input_type[s->k];
		s->new_num[s->p] = current->input_num[s->k];
		s->p = s->p + 1;
		s->k = s->k + 1;
	}
	return (0);
}

int	cy3_handle_dollar_word_2a(t_input *current, t_dollar_word *s)
{
	s->k = 0;
	while (s->k < s->vlen)
	{
		s->new_input[s->p] = s->value[s->k];
		s->new_type[s->p] = '5';
		s->new_num[s->p] = current->input_num[s->i];
		s->p = s->p + 1;
		s->k = s->k + 1;
	}
	return (0);
}

int	cy3_handle_dollar_word_2b(t_input *current, t_dollar_word *s)
{
	s->k = s->j + 1;
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


/*
** CORRECTION CRITIQUE - Initialisation manquante de s->p
**
** Problème : Valgrind révèle "Use of uninitialised value" dans cy3_handle_dollar_word_2a
** car s->p n'est jamais initialisé avant utilisation.
**
** Symptôme concret :
** - s->p contient une valeur garbage (ex: 0x6b877c10) 
** - cy3_handle_dollar_word_2a fait : s->new_input[s->p] = s->value[s->k]
** - Accès mémoire à une adresse invalide -> Segfault
**
** Exemple : Pour "Hello $HOME World"
** - s->p devrait être 0 pour commencer à écrire au début de new_input
** - Sans initialisation, s->p = garbage -> crash immédiat
**
** Solution : Initialiser s->p = 0 avant toute utilisation pour que
** l'écriture commence correctement au début du buffer alloué.
*/
int	cy3_handle_dollar_word_2(t_input *current, t_dollar_word *s)
{
	s->vlen = cy_strlen(s->value);
	s->lold = cy_strlen(current->input);
	s->new_input = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	s->new_type = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	s->new_num = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	if (!s->new_input || !s->new_type || !s->new_num)
		return (-1);
	s->p = 0;
	cy3_handle_dollar_word_2_before(current, s);
	cy3_handle_dollar_word_2a(current, s);
	cy3_handle_dollar_word_2b(current, s);
	s->new_input[s->p] = '\0';
	s->new_type[s->p] = '\0';
	s->new_num[s->p] = '\0';
	free(current->input);
	free(current->input_type);
	free(current->input_num);
	current->input = s->new_input;
	current->input_type = s->new_type;
	current->input_num = s->new_num;
	return (s->i + s->vlen - 1);
}

/*
int	cy3_handle_dollar_word_2(t_input *current, t_dollar_word *s)
{
	printf("DEBUG: Entering cy3_handle_dollar_word_2, s->i=%d, s->j=%d\n", s->i, s->j);
	s->vlen = cy_strlen(s->value);
	printf("DEBUG: s->vlen=%d, s->value='%s'\n", s->vlen, s->value);
	s->lold = cy_strlen(current->input);
	printf("DEBUG: s->lold=%d\n", s->lold);
	s->new_input = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	s->new_type = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	s->new_num = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	printf("DEBUG: malloc done, checking pointers\n");
	if (!s->new_input || !s->new_type || !s->new_num)
		return (-1);
	printf("DEBUG: About to call cy3_handle_dollar_word_2a\n");
	cy3_handle_dollar_word_2a(current, s);
	printf("DEBUG: 2a done, calling 2b\n");
	cy3_handle_dollar_word_2b(current, s);
	printf("DEBUG: 2b done\n");
	s->new_input[s->p] = '\0';
	s->new_type[s->p] = '\0';
	s->new_num[s->p] = '\0';
	free(current->input);
	free(current->input_type);
	free(current->input_num);
	current->input = s->new_input;
	current->input_type = s->new_type;
	current->input_num = s->new_num;
	return (s->i + s->vlen - 1);
}


int	cy3_handle_dollar_word_2(t_input *current, t_dollar_word *s)
{
	s->vlen = cy_strlen(s->value);
	s->lold = cy_strlen(current->input);
	s->new_input = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	s->new_type = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	s->new_num = malloc(s->lold - (s->j - s->i + 1) + s->vlen + 1);
	if (!s->new_input || !s->new_type || !s->new_num)
		return (-1);
	cy3_handle_dollar_word_2a(current, s);
	cy3_handle_dollar_word_2b(current, s);
	s->new_input[s->p] = '\0';
	s->new_type[s->p] = '\0';
	s->new_num[s->p] = '\0';
	free(current->input);
	free(current->input_type);
	free(current->input_num);
	current->input = s->new_input;
	current->input_type = s->new_type;
	current->input_num = s->new_num;
	return (s->i + s->vlen - 1);
}
*/
