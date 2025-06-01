#include "../prser.h"

void	cy3_handle_dollar_bang5(t_dollar_bang *sdb, t_input *current, int j)
{
	sdb->k = j + 1;
	while (current->input[sdb->k])
	{
		sdb->new_input[sdb->p] = current->input[sdb->k];
		sdb->new_type[sdb->p] = current->input_type[sdb->k];
		sdb->new_num[sdb->p] = current->input_num[sdb->k];
		sdb->k = sdb->k + 1;
		sdb->p = sdb->p + 1;
	}
}

void	cy3_handle_dollar_bang3(t_dollar_bang *sdb, t_input *current, int i)
{
	sdb->p = 0;
	sdb->k = 0;
	while (sdb->k < i)
	{
		sdb->new_input[sdb->p] = current->input[sdb->k];
		sdb->new_type[sdb->p] = current->input_type[sdb->k];
		sdb->new_num[sdb->p] = current->input_num[sdb->k];
		sdb->p = sdb->p + 1;
		sdb->k = sdb->k + 1;
	}
	sdb->k = 0;
	while (sdb->k < sdb->vlen)
	{
		sdb->new_input[sdb->p] = sdb->last_env[sdb->k];
		sdb->new_type[sdb->p] = '5';
		sdb->new_num[sdb->p] = current->input_num[i];
		sdb->k = sdb->k + 1;
		sdb->p = sdb->p + 1;
	}
}

void	cy3_handle_dollar_bang2(t_dollar_bang *sdb,
								t_input *current, int i, int j)
{
	sdb->vlen = cy_strlen(sdb->last_env);
	sdb->lold = cy_strlen(current->input);
	sdb->replaced_len = j - i + 1;
	sdb->new_input = malloc(sdb->lold - sdb->replaced_len + sdb->vlen + 1);
	sdb->new_type = malloc(sdb->lold - sdb->replaced_len + sdb->vlen + 1);
	sdb->new_num = malloc(sdb->lold - sdb->replaced_len + sdb->vlen + 1);
}

int	cy3_handle_dollar_bang1(t_dollar_bang *sdb, int exit_code)
{
	static char	exit_str[12];
	char		*temp_str;

	temp_str = ft_itoa(exit_code);
	if (!temp_str)
		return (0);
	cy_strlcpy(exit_str, temp_str, 12);
	free(temp_str);
	sdb->last_env = exit_str;
	return (1);
}

int	cy3_handle_dollar_bang(t_input *current, int i, int j, int exit_code)
{
	t_dollar_bang	sdb;

	if (!cy3_handle_dollar_bang1(&sdb, exit_code))
		return (-2);
	cy3_handle_dollar_bang2(&sdb, current, i, j);
	if (!sdb.new_input || !sdb.new_type || !sdb.new_num)
		return (-1);
	cy3_handle_dollar_bang3(&sdb, current, i);
	cy3_handle_dollar_bang5(&sdb, current, j);
	sdb.new_input[sdb.p] = '\0';
	sdb.new_type[sdb.p] = '\0';
	sdb.new_num[sdb.p] = '\0';
	free(current->input);
	free(current->input_type);
	free(current->input_num);
	current->input = sdb.new_input;
	current->input_type = sdb.new_type;
	current->input_num = sdb.new_num;
	return (i - 1 + sdb.vlen);
}
