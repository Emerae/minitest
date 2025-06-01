#include "../../includes/minishell.h"

char	*expand_single_tilde(char *arg, char **env)
{
	char	*home;
	char	*expanded;
	int		home_len;
	int		arg_len;

	if (!arg || arg[0] != '~')
		return (ft_strdup(arg));
	home = get_env_value(env, "HOME");
	if (!home)
		return (ft_strdup(arg));
	if (arg[1] == '\0')
		return (ft_strdup(home));
	if (arg[1] == '/')
	{
		home_len = ft_strlen(home);
		arg_len = ft_strlen(arg + 1);
		expanded = malloc(home_len + arg_len + 1);
		if (!expanded)
			return (NULL);
		ft_strcpy(expanded, home);
		ft_strcat(expanded, arg + 1);
		return (expanded);
	}
	return (ft_strdup(arg));
}

char	**expand_args_tildes(char **args, char **env)
{
	char	**expanded_args;
	int		count;
	int		i;

	if (!args)
		return (NULL);
	count = count_string_array(args);
	expanded_args = malloc(sizeof(char *) * (count + 1));
	if (!expanded_args)
		return (NULL);
	i = 0;
	while (i < count)
	{
		expanded_args[i] = expand_single_tilde(args[i], env);
		if (!expanded_args[i])
		{
			free_string_array(expanded_args);
			return (NULL);
		}
		i++;
	}
	expanded_args[i] = NULL;
	return (expanded_args);
}
