#include "../prser.h"


/*
** CORRECTION CRITIQUE - Bug de validation des caractères dans les quotes
**
** Problème : cy4_1wrong_char vérifie les caractères interdits dans TOUS les 
** nœuds, même ceux qui sont entre guillemets ! Cela viole les règles de bash.
**
** Selon le sujet minishell :
** - Handle ' (single quote) which should prevent the shell from interpreting 
**   the meta-characters in the quoted sequence.
** - Handle " (double quote) which should prevent the shell from interpreting 
**   the meta-characters in the quoted sequence except for $ (dollar sign).
**
** Exemples concrets du problème :
** 1. echo "test\ntest" -> ERREUR (backslash détecté)
** 2. echo 'test&test'  -> ERREUR (& détecté)  
** 3. echo "test*test"  -> ERREUR (* détecté)
**
** Alors que bash/minishell doivent accepter ces commandes car les caractères
** spéciaux sont PROTÉGÉS par les guillemets !
**
** Types de nœuds (voir parser.h) :
** - type 3 : contenu entre guillemets simples '' 
** - type 4 : contenu entre guillemets doubles ""
**
** Dans ces types, les caractères interdits deviennent des caractères littéraux
** et ne doivent PAS être rejetés par la validation.
**
** Conséquence sans cette correction : Impossible d'utiliser des backslashes,
** astérisques, etc. dans les chaînes quotées, ce qui casse la compatibilité bash.

** CORRECTION CRITIQUE - Bug de validation après expansion de variables
**
** Problème : Après expansion de $PATH (ou toute variable), les caractères
** résultants sont vérifiés comme caractères interdits, ce qui cause des 
** erreurs de syntaxe inappropriées.
**
** Exemple concret :
** - echo $PATH -> $PATH devient "/home/user/.local/bin:/usr/local/sbin:..."
** - Cette chaîne contient ':' et '/' qui sont dans les caractères interdits
** - cy4_1wrong_char rejette la commande avec "syntax error"
** - Alors que bash accepte parfaitement cette commande
**
** Solution : Les caractères provenant d'expansions de variables (type '5' 
** dans input_type) doivent être traités comme littéraux et exemptés de
** la vérification des caractères interdits.
**
** Types dans input_type :
** - '5' : caractère provenant d'une expansion de variable (voir cy3_handle_dollar_word_2a)
*/

int	cy4_1wrong_char(t_input *head)
{
	t_input	*current;
	int		i;

	current = head;
	while (current)
	{
		if (current->input)
		{
			if (current->type == 3 || current->type == 4)
			{
				current = current->next;
				continue ;
			}
			i = 0;
			while (current->input[i])
			{
				if (current->input_type && current->input_type[i] == '5')
				{
					i = i + 1;
					continue ;
				}
				if (cy0_analyse_char2(current->input[i]) == 1)
					return (1);
				if (cy0_analyse_char2(current->input[i]) == -14)
					return (2);
				i = i + 1;
			}
		}
		current = current->next;
	}
	return (0);
}

/*
int	cy4_1wrong_char(t_input *head)
{
	t_input	*current;
	int		i;

	current = head;
	while (current)
	{
		if (current->input)
		{
			i = 0;
			while (current->input[i])
			{
				if (cy0_analyse_char2(current->input[i]) == 1)
					return (1);
				if (cy0_analyse_char2(current->input[i]) == -14)
					return (2);
				i = i + 1;
			}
		}
		current = current->next;
	}
	return (0);
}
*/
