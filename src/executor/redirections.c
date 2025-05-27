
#include "../../includes/minishell.h"

/*
** OUVERTURE D'UN FICHIER POUR UNE REDIRECTION
** Cette fonction ouvre un fichier avec les bons flags selon le type de redirection.
** 
** Types de redirections et leurs flags :
** - Type 0 (<) : lecture seule - O_RDONLY
** - Type 1 (>) : écriture, écrase le fichier - O_WRONLY | O_CREAT | O_TRUNC
** - Type 2 (>>) : écriture, ajoute à la fin - O_WRONLY | O_CREAT | O_APPEND
** 
** Permissions 0644 pour les nouveaux fichiers :
** - Propriétaire : lecture + écriture (6 = 4+2)
** - Groupe : lecture seule (4)
** - Autres : lecture seule (4)
** 
** Gestion d'erreur : si open() échoue, on affiche l'erreur système avec perror.
** Exemples d'erreurs courantes :
** - "Permission denied" : pas les droits d'écriture
** - "No such file or directory" : fichier inexistant pour <
** - "Is a directory" : on essaie d'écrire dans un dossier
** 
** Paramètres :
** - filename : nom du fichier à ouvrir
** - type : type de redirection (0=<, 1=>, 2=>>)
** 
** Retourne : descripteur de fichier si succès, -1 si erreur
*/
int	open_file_for_redirect(char *filename, int type)
{
	int	fd;

	if (type == 0)
		fd = open(filename, O_RDONLY);
	else if (type == 1)
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (type == 2)
		fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		return (-1);
	if (fd == -1)
	{
		write(STDERR_FILENO, "minishell: ", 11);
		write(STDERR_FILENO, filename, ft_strlen(filename));
		write(STDERR_FILENO, ": ", 2);
		perror("");
	}
	return (fd);
}

/*
** GESTION DES INTERRUPTIONS PENDANT UN HEREDOC
** Cette fonction traite le cas où l'utilisateur tape Ctrl-C pendant un heredoc.
** 
** Que faire quand on reçoit Ctrl-C dans un heredoc ?
** 1. Remettre g_signal_received à 0 (on a traité le signal)
** 2. Fermer les descripteurs du pipe (nettoyer)
** 3. Remettre les signaux en mode normal
** 4. Retourner -1 pour signaler l'annulation
** 
** Pourquoi fermer les deux bouts du pipe ?
** Si on laisse un bout ouvert, le processus qui lit pourrait rester bloqué
** indéfiniment en attendant des données qui ne viendront jamais.
** 
** Paramètres :
** - pipe_fd : tableau des descripteurs du pipe [lecture, écriture]
** 
** Retourne : -1 (toujours, pour indiquer l'annulation)
*/
static int	handle_heredoc_interruption(int pipe_fd[2])
{
	if (g_signal_received == SIGINT)
	{
		g_signal_received = 0;
		close(pipe_fd[1]);
		close(pipe_fd[0]);
		setup_signals();
		return (-1);
	}
	return (0);
}

/*
** TRAITEMENT D'UNE LIGNE DANS UN HEREDOC
** Cette fonction traite une ligne lue pendant un heredoc et décide quoi en faire.
** 
** Logique :
** 1. Si la ligne est égale au délimiteur : c'est fini, on sort
** 2. Sinon : on écrit la ligne dans le pipe + un \n
** 3. On libère la mémoire de la ligne dans tous les cas
** 
** Exemple pour "cat << EOF" :
** - Délimiteur = "EOF"
** - Ligne "hello" → écrite dans le pipe avec \n
** - Ligne "EOF" → fin du heredoc, pas écrite
** 
** Pourquoi ajouter \n ? Parce que readline() enlève le \n automatiquement,
** mais on veut le garder dans le contenu final.
** 
** Paramètres :
** - line : ligne lue par readline()
** - delimiter : délimiteur de fin ("EOF", "END", etc.)
** - pipe_fd : pipe où écrire le contenu
** 
** Retourne : 1 si délimiteur trouvé (fin), 0 si ligne normale
*/
static int	process_heredoc_line(char *line, char *delimiter, int pipe_fd[2])
{
	if (ft_strcmp(line, delimiter) == 0)
	{
		free(line);
		return (1);
	}
	write(pipe_fd[1], line, ft_strlen(line));
	write(pipe_fd[1], "\n", 1);
	free(line);
	return (0);
}

/*
** BOUCLE DE LECTURE D'UN HEREDOC
** Cette fonction lit les lignes d'un heredoc jusqu'au délimiteur ou interruption.
** 
** Boucle infinie qui :
** 1. Lit une ligne avec readline("> ")
** 2. Vérifie si Ctrl-C ou EOF (readline retourne NULL)
** 3. Traite la ligne (écriture dans pipe ou comparaison délimiteur)
** 4. Continue jusqu'à délimiteur trouvé ou interruption
** 
** Gestion des cas particuliers :
** - readline() retourne NULL : soit EOF (Ctrl-D), soit Ctrl-C
** - Si g_signal_received == SIGINT : c'est un Ctrl-C
** - Sinon : c'est un EOF, on sort normalement
** 
** Exemple d'interaction :
** $ cat << EOF
** > ligne 1
** > ligne 2  
** > EOF
** → "ligne 1\nligne 2\n" est écrit dans le pipe
** 
** Paramètres :
** - delimiter : chaîne de fin du heredoc
** - pipe_fd : pipe où écrire le contenu lu
** 
** Retourne : 0 si succès, -1 si Ctrl-C
*/
static int	read_heredoc_loop(char *delimiter, int pipe_fd[2])
{
	char	*line;
	int		line_result;

	while (1)
	{
		line = readline("> ");
		if (!line || g_signal_received == SIGINT)
		{
			if (handle_heredoc_interruption(pipe_fd) == -1)
				return (-1);
			break ;
		}
		line_result = process_heredoc_line(line, delimiter, pipe_fd);
		if (line_result == 1)
			break ;
		if (line_result == -1)
			return (-1);
	}
	return (0);
}

/*
** GESTION COMPLÈTE D'UN HEREDOC
** Cette fonction orchestre tout le processus d'un heredoc (<<).
** 
** Un heredoc permet de passer du texte multi-lignes à une commande :
** cat << EOF
** ligne 1
** ligne 2
** EOF
** 
** Processus :
** 1. Crée un pipe (tuyau mémoire)
** 2. Configure les signaux pour le mode heredoc (Ctrl-C spécial)
** 3. Lit les lignes utilisateur et les écrit dans le pipe
** 4. Ferme le bout écriture du pipe
** 5. Remet les signaux en mode normal
** 6. Retourne le bout lecture du pipe
** 
** La commande pourra ensuite lire le contenu depuis ce descripteur.
** 
** Avantage du pipe : pas besoin de fichier temporaire sur le disque.
** Le contenu reste en mémoire, c'est plus rapide et plus propre.
** 
** Paramètres :
** - delimiter : chaîne qui marque la fin du heredoc
** 
** Retourne : descripteur de lecture du pipe, ou -1 si erreur/Ctrl-C
*/
int	handle_heredoc(char *delimiter)
{
	int	pipe_fd[2];

	if (pipe(pipe_fd) == -1)
		return (-1);
	setup_heredoc_signals();
	if (read_heredoc_loop(delimiter, pipe_fd) == -1)
		return (-1);
	close(pipe_fd[1]);
	setup_signals();
	return (pipe_fd[0]);
}

/*
** APPLICATION D'UNE REDIRECTION UNIQUE
** Cette fonction applique une seule redirection à la commande courante.
** 
** Types de redirections :
** - 0 et 3 (<, <<) : redirection d'entrée → stdin
** - 1 et 2 (>, >>) : redirection de sortie → stdout
** 
** Processus :
** 1. Ouvre le fichier ou gère le heredoc selon le type
** 2. Utilise dup2() pour rediriger stdin ou stdout vers ce descripteur
** 3. Ferme le descripteur original (plus besoin)
** 
** dup2() c'est quoi ? 
** dup2(fd, STDOUT_FILENO) fait que STDOUT_FILENO pointe maintenant vers fd.
** Résultat : tous les printf/write vers stdout vont maintenant dans fd.
** 
** Exemple pour "ls > fichier.txt" :
** 1. open("fichier.txt", O_WRONLY | O_CREAT | O_TRUNC) → fd=3
** 2. dup2(3, STDOUT_FILENO) → stdout pointe vers fichier.txt
** 3. close(3) → on ferme l'original
** 4. ls écrit dans stdout → ça va dans fichier.txt
** 
** Paramètres :
** - redir : structure contenant le type et le nom de fichier
** 
** Retourne : 0 si succès, -1 si erreur
*/
static int	apply_redirection(t_redir *redir)
{
	int	fd;
	int	target_fd;

	if (redir->type == 3)
		fd = handle_heredoc(redir->file);
	else
		fd = open_file_for_redirect(redir->file, redir->type);
	if (fd == -1)
		return (-1);
	if (redir->type == 0 || redir->type == 3)
		target_fd = STDIN_FILENO;
	else
		target_fd = STDOUT_FILENO;
	if (dup2(fd, target_fd) == -1)
	{
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

/*
** MISE EN PLACE DE TOUTES LES REDIRECTIONS D'UNE COMMANDE
** Cette fonction applique toutes les redirections d'une commande dans l'ordre.
** 
** Une commande peut avoir plusieurs redirections :
** cat < input.txt > output.txt 2> errors.txt
** 
** Ordre d'application important ! Les redirections sont traitées de gauche à droite.
** Exemple : "echo hello > file1 > file2"
** 1. Redirige stdout vers file1
** 2. Redirige stdout vers file2 (écrase la première redirection)
** Résultat : "hello" va dans file2, file1 reste vide
** 
** C'est le même comportement que bash !
** 
** Gestion d'erreur : si une redirection échoue, on s'arrête immédiatement.
** Les redirections déjà appliquées restent en place (pas de rollback).
** 
** Cas d'usage :
** - Commande simple : "ls > file"
** - Pipeline : "ls | grep test > file" (seulement grep a une redirection)
** - Complexe : "cmd < in > out 2> err"
** 
** Paramètres :
** - redirs : liste chaînée des redirections à appliquer
** 
** Retourne : 0 si toutes les redirections ont réussi, -1 si erreur
*/
int	setup_redirections(t_redir *redirs)
{
	t_redir	*current;

	current = redirs;
	while (current)
	{
		if (apply_redirection(current) == -1)
			return (-1);
		current = current->next;
	}
	return (0);
}

/*
#include "../../includes/minishell.h"

int	open_file_for_redirect(char *filename, int type)
{
	int	fd;

	if (type == 0)
		fd = open(filename, O_RDONLY);
	else if (type == 1)
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (type == 2)
		fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		return (-1);
	if (fd == -1)
	{
		write(STDERR_FILENO, "minishell: ", 11);
		write(STDERR_FILENO, filename, ft_strlen(filename));
		write(STDERR_FILENO, ": ", 2);
		perror("");
	}
	return (fd);
}

static int	handle_heredoc_interruption(int pipe_fd[2])
{
	if (g_signal_received == SIGINT)
	{
		g_signal_received = 0;
		close(pipe_fd[1]);
		close(pipe_fd[0]);
		setup_signals();
		return (-1);
	}
	return (0);
}

static int	process_heredoc_line(char *line, char *delimiter, int pipe_fd[2])
{
	if (ft_strcmp(line, delimiter) == 0)
	{
		free(line);
		return (1);
	}
	write(pipe_fd[1], line, ft_strlen(line));
	write(pipe_fd[1], "\n", 1);
	free(line);
	return (0);
}

static int	read_heredoc_loop(char *delimiter, int pipe_fd[2])
{
	char	*line;
	int		line_result;

	while (1)
	{
		line = readline("> ");
		if (!line || g_signal_received == SIGINT)
		{
			if (handle_heredoc_interruption(pipe_fd) == -1)
				return (-1);
			break ;
		}
		line_result = process_heredoc_line(line, delimiter, pipe_fd);
		if (line_result == 1)
			break ;
		if (line_result == -1)
			return (-1);
	}
	return (0);
}

int	handle_heredoc(char *delimiter)
{
	int	pipe_fd[2];

	if (pipe(pipe_fd) == -1)
		return (-1);
	setup_heredoc_signals();
	if (read_heredoc_loop(delimiter, pipe_fd) == -1)
		return (-1);
	close(pipe_fd[1]);
	setup_signals();
	return (pipe_fd[0]);
}

static int	apply_redirection(t_redir *redir)
{
	int	fd;
	int	target_fd;

	if (redir->type == 3)
		fd = handle_heredoc(redir->file);
	else
		fd = open_file_for_redirect(redir->file, redir->type);
	if (fd == -1)
		return (-1);
	if (redir->type == 0 || redir->type == 3)
		target_fd = STDIN_FILENO;
	else
		target_fd = STDOUT_FILENO;
	if (dup2(fd, target_fd) == -1)
	{
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

int	setup_redirections(t_redir *redirs)
{
	t_redir	*current;

	current = redirs;
	while (current)
	{
		if (apply_redirection(current) == -1)
			return (-1);
		current = current->next;
	}
	return (0);
}
*/
