#include "../../includes/minishell.h"

char	**copy_env(char **envp)
{
	char	**new_env;
	int		i;
	int		count;

	if (!envp)
		return (NULL);
	count = 0;
	while (envp[count])
		count++;
	new_env = malloc(sizeof(char *) * (count + 1));
	if (!new_env)
		return (NULL);
	i = 0;
	while (i < count)
	{
		new_env[i] = ft_strdup(envp[i]);
		if (!new_env[i])
		{
			free_env(new_env);
			return (NULL);
		}
		i++;
	}
	new_env[i] = NULL;
	return (new_env);
}

void	free_env(char **env)
{
	int	i;

	if (!env)
		return ;
	i = 0;
	while (env[i])
	{
		free(env[i]);
		i++;
	}
	free(env);
}

char	*get_env_value(char **env, char *key)
{
	int		i;
	int		key_len;
	char	*equal_sign;

	if (!env || !key)
		return (NULL);
	key_len = ft_strlen(key);
	i = 0;
	while (env[i])
	{
		equal_sign = ft_strchr(env[i], '=');
		if (equal_sign && (equal_sign - env[i]) == key_len
			&& ft_strncmp(env[i], key, key_len) == 0)
		{
			return (equal_sign + 1);
		}
		i++;
	}
	return (NULL);
}

char	**realloc_env(char **env, int new_size)
{
	char	**new_env;
	int		i;

	new_env = malloc(sizeof(char *) * (new_size + 1));
	if (!new_env)
		return (NULL);
	i = 0;
	while (env[i] && i < new_size)
	{
		new_env[i] = env[i];
		i++;
	}
	while (i <= new_size)
	{
		new_env[i] = NULL;
		i++;
	}
	free(env);
	return (new_env);
}

static int	find_env_index(char **env, char *key)
{
	int		i;
	int		key_len;
	char	*equal_sign;

	if (!env || !key)
		return (-1);
	key_len = ft_strlen(key);
	i = 0;
	while (env[i])
	{
		equal_sign = ft_strchr(env[i], '=');
		if (equal_sign && (equal_sign - env[i]) == key_len
			&& ft_strncmp(env[i], key, key_len) == 0)
			return (i);
		i++;
	}
	return (-1);
}

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


