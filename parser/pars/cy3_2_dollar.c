#include "../prser.h"

/*
** MODIFICATION CRITIQUE - Support de $? avec exit code  
**
** Problème : $? (sans accolades) utilisait aussi la dernière variable
** d'environnement au lieu du code de sortie.
**
** Exemple du bug :
** - minishell$ ls (exit code = 0)  
** - minishell$ echo $?
** - Résultat : "_=./minishell" au lieu de "0"
**
** Solution : Propager exit_code vers cy3_handle_dollar_bang pour $?
** seulement, les autres variables restent avec env
*/
int	cy3_scan_dollar_syntax_2(t_input *current, int i, char **env, int exit_code)
{
	int	j;

	if (current->input[i + 1] == '?')
	{
		if (current->input_num[i + 1] != current->input_num[i])
			return (1);
		j = i + 1;
		printf("i = %d\n j = %d\n", i, j);
		i = cy3_handle_dollar_bang(current, i, j, env, exit_code);
		printf("ii = %d\njj = %d\n", i, j);
		if (i == -1)
			return (1);
		return (0);
	}
	return (cy3_scan_dollar_syntax_2_1(current, i, env));
}

/*
int	cy3_scan_dollar_syntax_2(t_input *current, int i, char **env)
{
	int	j;

	if (current->input[i + 1] == '?')
	{
		if (current->input_num[i + 1] != current->input_num[i])
			return (1);
		j = i + 1;
		printf("i = %d\n j = %d\n", i, j);
		i = cy3_handle_dollar_bang(current, i, j, env);
		printf("ii = %d\njj = %d\n", i, j);
		if (i == -1)
			return (1);
		return (0);
	}
	return (cy3_scan_dollar_syntax_2_1(current, i, env));
}
*/

/*
** MODIFICATION CRITIQUE - Support de ${?} avec exit code
**
** Problème : ${?} utilisait la dernière variable d'environnement au lieu
** du code de sortie de la dernière commande.
**
** Exemple du bug :
** - minishell$ ls (exit code = 0)
** - minishell$ echo ${?} 
** - Résultat : "_=./minishell" au lieu de "0"
**
** Solution : Propager exit_code vers cy3_handle_dollar_bang pour ${?}
** tout en gardant env pour les autres expansions ${VAR}
*/
int	cy3_scan_dollar_syntax_dollar_1(t_input *current, int *i, char **env, int exit_code)
{
	if (current->input[*i + 2] && current->input[*i + 3]
		&& current->input[*i + 3] == '}' && current->input[*i + 2] == '?'
		&& current->input_num[*i] == current->input_num[*i + 1]
		&& current->input_num[*i + 1] == current->input_num[*i + 2]
		&& current->input_num[*i + 2] == current->input_num[*i + 3])
	{
		printf("found ${?}\n");
		printf("i = %d\n", *i);
		*i = cy3_handle_dollar_bang(current, *i, *i + 3, env, exit_code);
		printf("ii = %d\n", *i);
		if (*i == -1)
			return (1);
		*i = *i + 1;
		return (0);
	}
	if (cy3_scan_dollar_syntax_1_1(current, *i, env))
		return (1);
	return (0);
}

/*
int	cy3_scan_dollar_syntax_dollar_1(t_input *current,
		int *i, char **env)
{
	if (current->input[*i + 2] && current->input[*i + 3]
		&& current->input[*i + 3] == '}' && current->input[*i + 2] == '?'
		&& current->input_num[*i] == current->input_num[*i + 1]
		&& current->input_num[*i + 1] == current->input_num[*i + 2]
		&& current->input_num[*i + 2] == current->input_num[*i + 3])
	{
		printf("found ${?}\n");
		printf("i = %d\n", *i);
		*i = cy3_handle_dollar_bang(current, *i, *i + 3, env);
		printf("ii = %d\n", *i);
		if (*i == -1)
			return (1);
		*i = *i + 1;
		return (0);
	}
	if (cy3_scan_dollar_syntax_1_1(current, *i, env))
		return (1);
	return (0);
}
*/

/*
** MODIFICATION CRITIQUE - Propagation de l'exit code pour $? 
**
** Problème : cy3_scan_dollar_syntax_dollar_2 peut traiter $? (quand le caractère
** après $ est '?') mais n'avait pas accès à exit_code.
**
** Solution : Ajouter exit_code en paramètre et le propager vers
** cy3_scan_dollar_syntax_2 qui gère les cas $? et $VAR
*/
int	cy3_scan_dollar_syntax_dollar_2(t_input *current,
		int *i, char **env, int exit_code)
{
	if ((current->input[*i + 1] >= 'A' && current->input[*i + 1] <= 'Z')
		|| (current->input[*i + 1] >= 'a' && current->input[*i + 1] <= 'z')
		|| current->input[*i + 1] == '_'
		|| current->input[*i + 1] == '?')
	{
		if (cy3_scan_dollar_syntax_2(current, *i, env, exit_code))
			return (1);
	}
	else
	{
		*i = cy3_handle_dollar_alone(current, *i);
		if (*i == -1)
			return (1);
	}
	return (0);
}

/*
int	cy3_scan_dollar_syntax_dollar_2(t_input *current,
		int *i, char **env)
{
	if ((current->input[*i + 1] >= 'A' && current->input[*i + 1] <= 'Z')
		|| (current->input[*i + 1] >= 'a' && current->input[*i + 1] <= 'z')
		|| current->input[*i + 1] == '_'
		|| current->input[*i + 1] == '?')
	{
		if (cy3_scan_dollar_syntax_2(current, *i, env))
			return (1);
	}
	else
	{
		*i = cy3_handle_dollar_alone(current, *i);
		if (*i == -1)
			return (1);
	}
	return (0);
}
*/

/*
** MODIFICATION CRITIQUE - Propagation sélective de l'exit code
**
** Problème : Seules les expansions de $? ont besoin de l'exit code.
** Les expansions normales de variables ($PATH, etc.) utilisent toujours env.
**
** Logic : 
** - Si c'est ${?} -> appeler dollar_1 avec exit_code
** - Si c'est $var -> appeler dollar_2 avec env seulement
** - Ainsi, on ne casse rien dans l'expansion normale des variables
*/
int	cy3_scan_dollar_syntax_dollar(t_input *current, int *i, char **env, int exit_code)
{
	if (current->input[*i + 1] == '\0'
		|| current->input_num[*i + 1] != current->input_num[*i])
	{
		*i = *i + 1;
		return (0);
	}
	else if (current->input[*i + 1] == '{')
		return (cy3_scan_dollar_syntax_dollar_1(current, i, env, exit_code));
	else
		return (cy3_scan_dollar_syntax_dollar_2(current, i, env, exit_code));
	return (0);
}

/*
int	cy3_scan_dollar_syntax_dollar(t_input *current, int *i, char **env)
{
	if (current->input[*i + 1] == '\0'
		|| current->input_num[*i + 1] != current->input_num[*i])
	{
		*i = *i + 1;
		return (0);
	}
	else if (current->input[*i + 1] == '{')
		return (cy3_scan_dollar_syntax_dollar_1(current, i, env));
	else
		return (cy3_scan_dollar_syntax_dollar_2(current, i, env));
	return (0);
}
*/

/*
** MODIFICATION CRITIQUE - Propagation de l'exit code dans le parser
**
** Problème : cy3_scan_dollar_syntax doit maintenant recevoir et propager
** l'exit code pour que $? fonctionne correctement.
**
** Seul changement : Ajout du paramètre exit_code et propagation vers
** cy3_scan_dollar_syntax_dollar. Aucune autre logique modifiée.
*/
int	cy3_scan_dollar_syntax(t_input *head, char **env, int exit_code)
{
	t_input	*current;
	int		i;

	current = head;
	while (current)
	{
		i = 0;
		while (current->input && current->input[i])
		{
			if (current->input[i] == '$' && current->type != 3)
			{
				if (cy3_scan_dollar_syntax_dollar(current, &i, env, exit_code))
					return (1);
				continue ;
			}
			i = i + 1;
		}
		current = current->next;
	}
	return (0);
}

/*
int	cy3_scan_dollar_syntax(t_input *head, char **env)
{
	t_input	*current;
	int		i;

	current = head;
	while (current)
	{
		i = 0;
		while (current->input && current->input[i])
		{
			if (current->input[i] == '$' && current->type != 3)
			{
				if (cy3_scan_dollar_syntax_dollar(current, &i, env))
					return (1);
				continue ;
			}
			i = i + 1;
		}
		current = current->next;
	}
	return (0);
}
*/
