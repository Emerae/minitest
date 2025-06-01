#include "../../includes/minishell.h"

static char	*create_env_entry(char *key, char *value)
{
	char	*new_entry;

	new_entry = malloc(ft_strlen(key) + ft_strlen(value) + 2);
	if (!new_entry)
		return (NULL);
	ft_strcpy(new_entry, key);
	ft_strcat(new_entry, "=");
	if (value)
		ft_strcat(new_entry, value);
	return (new_entry);
}

static int	update_existing_env(char ***env, char *new_entry, int index)
{
	free((*env)[index]);
	(*env)[index] = new_entry;
	return (0);
}

static int	add_new_env_entry(char ***env, char *new_entry)
{
	char	**new_env;
	int		count;

	count = count_string_array(*env);
	new_env = realloc_env(*env, count + 1);
	if (!new_env)
	{
		free(new_entry);
		return (1);
	}
	new_env[count] = new_entry;
	new_env[count + 1] = NULL;
	*env = new_env;
	return (0);
}

int	set_env_value(char ***env, char *key, char *value)
{
	char	*new_entry;
	int		index;

	if (!env || !*env || !key)
		return (1);
	new_entry = create_env_entry(key, value);
	if (!new_entry)
		return (1);
	index = find_env_index(*env, key);
	if (index >= 0)
		return (update_existing_env(env, new_entry, index));
	return (add_new_env_entry(env, new_entry));
}

int	unset_env_value(char ***env, char *key)
{
	int	index;
	int	i;

	if (!env || !*env || !key)
		return (1);
	index = find_env_index(*env, key);
	if (index < 0)
		return (0);
	free((*env)[index]);
	i = index;
	while ((*env)[i + 1])
	{
		(*env)[i] = (*env)[i + 1];
		i++;
	}
	(*env)[i] = NULL;
	return (0);
}
