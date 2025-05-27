
#include "../../includes/minishell.h"

/*
** VÉRIFICATION SI UN FICHIER EST EXÉCUTABLE
** Cette fonction vérifie si un fichier donné existe et peut être exécuté.
** 
** Comment ça marche :
** 1. stat() récupère les infos du fichier (taille, permissions, type, etc.)
** 2. On vérifie que c'est un fichier régulier (pas un dossier, lien, etc.)
** 3. On vérifie qu'il a les permissions d'exécution pour le propriétaire
** 
** Utilisée pour : vérifier si "./mon_script" ou "/bin/ls" peut être exécuté.
** 
** Bits de permissions Unix :
** - S_ISREG : macro qui teste si c'est un fichier régulier
** - S_IXUSR : bit d'exécution pour le propriétaire (owner execute)
** 
** Paramètres :
** - path : chemin vers le fichier à tester (ex: "/bin/ls", "./script.sh")
** 
** Retourne : 1 si exécutable, 0 sinon
*/
static int	is_executable(char *path)
{
	struct stat	statbuf;
	
	if (stat(path, &statbuf) == -1)
		return (0);
	if (S_ISREG(statbuf.st_mode) && (statbuf.st_mode & S_IXUSR))
		return (1);
	return (0);
}

/*
** CONSTRUCTION D'UN CHEMIN COMPLET ET VÉRIFICATION
** Cette fonction combine un répertoire et un nom de commande pour former un chemin complet,
** puis vérifie si ce chemin est exécutable.
** 
** Exemple : dir="/usr/bin", cmd="ls" -> "/usr/bin/ls"
** 
** Étapes :
** 1. Calcule les tailles pour allouer la bonne quantité de mémoire
** 2. Copie le répertoire dans le nouveau string
** 3. Ajoute un '/' à la fin si nécessaire (certains paths finissent déjà par '/')
** 4. Ajoute le nom de la commande
** 5. Vérifie si le fichier résultant est exécutable
** 6. Si oui, retourne le chemin ; si non, libère la mémoire et retourne NULL
** 
** Gestion mémoire : si le fichier n'est pas exécutable, on free() pour éviter les leaks.
** 
** Paramètres :
** - dir : répertoire PATH (ex: "/usr/bin", "/bin")
** - cmd : nom de la commande (ex: "ls", "grep")
** 
** Retourne : chemin complet si exécutable, NULL sinon
*/
static char	*check_path(char *dir, char *cmd)
{
	char	*full_path;
	int		dir_len;
	int		cmd_len;

	dir_len = ft_strlen(dir);
	cmd_len = ft_strlen(cmd);
	full_path = malloc(dir_len + cmd_len + 2);
	if (!full_path)
		return (NULL);
	ft_strcpy(full_path, dir);
	if (dir[dir_len - 1] != '/')
		ft_strcat(full_path, "/");
	ft_strcat(full_path, cmd);
	if (is_executable(full_path))
		return (full_path);
	free(full_path);
	return (NULL);
}

/*
** RÉCUPÉRATION DES RÉPERTOIRES DU PATH
** Cette fonction extrait la variable d'environnement PATH et la découpe en tableau.
** 
** Le PATH c'est quoi ? Une liste de répertoires séparés par ':' où le shell
** va chercher les commandes. Exemple :
** PATH="/usr/bin:/bin:/usr/local/bin"
** 
** Processus :
** 1. Récupère la valeur de PATH dans l'environnement
** 2. Utilise ft_split() pour découper selon les ':'
** 3. Retourne un tableau de strings : ["/usr/bin", "/bin", "/usr/local/bin", NULL]
** 
** Cas d'erreur : si PATH n'existe pas, retourne NULL. Dans ce cas, on ne pourra
** exécuter que des commandes avec chemin absolu (/bin/ls) ou relatif (./script).
** 
** Paramètres :
** - env : tableau d'environnement du shell
** 
** Retourne : tableau des répertoires PATH, ou NULL si pas de PATH
*/
char	**get_paths_from_env(char **env)
{
	char	*path_value;
	char	**paths;

	path_value = get_env_value(env, "PATH");
	if (!path_value)
		return (NULL);
	paths = ft_split(path_value, ':');
	return (paths);
}

/*
** GESTION DES CHEMINS ABSOLUS ET RELATIFS
** Cette fonction traite les commandes qui contiennent déjà un chemin.
** 
** Exemples de chemins absolus/relatifs :
** - "/bin/ls" (absolu)
** - "./mon_script" (relatif au répertoire courant)
** - "../autre_script" (relatif au répertoire parent)
** - "dossier/script" (relatif)
** 
** Comment on les détecte ? Si la commande contient un '/', c'est qu'elle
** spécifie déjà un chemin, donc pas besoin de chercher dans PATH.
** 
** Vérifications :
** 1. Le fichier existe-t-il ? (stat() retourne 0)
** 2. Est-ce un fichier régulier ? (pas un dossier)
** 
** Note : on ne vérifie PAS les permissions ici, contrairement à is_executable().
** Bash permet d'essayer d'exécuter même sans permissions, et c'est execve()
** qui retournera l'erreur "Permission denied".
** 
** Paramètres :
** - cmd : commande avec chemin (ex: "./script", "/bin/ls")
** 
** Retourne : copie de cmd si le fichier existe, NULL sinon
*/
static char	*handle_absolute_path(char *cmd)
{
	struct stat	statbuf;

	if (stat(cmd, &statbuf) == -1)
		return (NULL);
	if (S_ISREG(statbuf.st_mode))
		return (ft_strdup(cmd));
	return (NULL);
}

/*
** RECHERCHE D'UNE COMMANDE DANS LE PATH
** Cette fonction cherche une commande dans tous les répertoires du PATH.
** 
** Algorithme :
** 1. Récupère la liste des répertoires PATH
** 2. Pour chaque répertoire :
**    a. Construit le chemin complet (répertoire + "/" + commande)
**    b. Vérifie si ce chemin est exécutable
**    c. Si oui : nettoie et retourne le chemin trouvé
**    d. Si non : continue avec le répertoire suivant
** 3. Si aucun répertoire ne contient la commande : retourne NULL
** 
** Exemple pour la commande "ls" :
** PATH="/usr/bin:/bin"  
** 1. Teste "/usr/bin/ls" -> pas trouvé
** 2. Teste "/bin/ls" -> trouvé et exécutable -> retourne "/bin/ls"
** 
** Gestion mémoire : on libère le tableau paths dans tous les cas (trouvé ou pas).
** 
** Paramètres :
** - cmd : nom de la commande à chercher (ex: "ls", "grep")
** - env : environnement pour récupérer PATH
** 
** Retourne : chemin complet vers l'exécutable, ou NULL si pas trouvé
*/
static char	*search_in_path(char *cmd, char **env)
{
	char	**paths;
	char	*cmd_path;
	int		i;

	paths = get_paths_from_env(env);
	if (!paths)
		return (NULL);
	i = 0;
	while (paths[i])
	{
		cmd_path = check_path(paths[i], cmd);
		if (cmd_path)
		{
			free_string_array(paths);
			return (cmd_path);
		}
		i++;
	}
	free_string_array(paths);
	return (NULL);
}

/*
** POINT D'ENTRÉE PRINCIPAL : TROUVER LE CHEMIN D'UNE COMMANDE
** Cette fonction détermine le chemin complet vers un exécutable, selon le type de commande.
** 
** Types de commandes gérées :
** 1. Commandes avec chemin (contiennent '/') :
**    - "/bin/ls", "./script", "../autre" 
**    -> appelle handle_absolute_path()
** 
** 2. Commandes simples (sans '/') :
**    - "ls", "grep", "cat"
**    -> appelle search_in_path() qui cherche dans PATH
** 
** Logique de détection : ft_strchr(cmd, '/') 
** - Retourne un pointeur si '/' trouvé (= chemin spécifié)
** - Retourne NULL si pas de '/' (= chercher dans PATH)
** 
** Cas d'erreur : commande vide ou NULL -> retourne NULL immédiatement.
** 
** Utilisation : appelée avant execve() pour savoir quel fichier exécuter.
** 
** Paramètres :
** - cmd : commande tapée par l'utilisateur
** - env : environnement du shell (pour PATH)
** 
** Retourne : chemin vers l'exécutable, ou NULL si pas trouvé
*/
char	*find_command_path(char *cmd, char **env)
{
	if (!cmd || !*cmd)
		return (NULL);
	if (ft_strchr(cmd, '/'))
		return (handle_absolute_path(cmd));
	return (search_in_path(cmd, env));
}

/*
** AFFICHAGE DES MESSAGES D'ERREUR DE COMMANDE
** Cette fonction affiche les erreurs de commande dans le format bash standard.
** 
** Format : "minishell: commande: message"
** Exemples :
** - "minishell: ls: command not found"
** - "minishell: ./script: Permission denied"
** - "minishell: /bin/ls: No such file or directory"
** 
** Pourquoi write() au lieu de printf() ?
** 1. Plus simple et direct
** 2. Pas de buffering (message affiché immédiatement)
** 3. Compatible avec les signaux (printf peut être interrompu)
** 
** Écrit sur STDERR (pas STDOUT) comme bash, pour que les erreurs
** ne soient pas mélangées avec la sortie normale des commandes.
** 
** Paramètres :
** - cmd : nom de la commande qui a échoué
** - msg : message d'erreur ("command not found", "Permission denied", etc.)
*/
void	print_error(char *cmd, char *msg)
{
	write(STDERR_FILENO, "minishell: ", 11);
	write(STDERR_FILENO, cmd, ft_strlen(cmd));
	write(STDERR_FILENO, ": ", 2);
	write(STDERR_FILENO, msg, ft_strlen(msg));
	write(STDERR_FILENO, "\n", 1);
}


/*
#include "../../includes/minishell.h"

static int	is_executable(char *path)
{
	struct stat	statbuf;
	
	if (stat(path, &statbuf) == -1)
		return (0);
	if (S_ISREG(statbuf.st_mode) && (statbuf.st_mode & S_IXUSR))
		return (1);
	return (0);
}

static char	*check_path(char *dir, char *cmd)
{
	char	*full_path;
	int		dir_len;
	int		cmd_len;

	dir_len = ft_strlen(dir);
	cmd_len = ft_strlen(cmd);
	full_path = malloc(dir_len + cmd_len + 2);
	if (!full_path)
		return (NULL);
	ft_strcpy(full_path, dir);
	if (dir[dir_len - 1] != '/')
		ft_strcat(full_path, "/");
	ft_strcat(full_path, cmd);
	if (is_executable(full_path))
		return (full_path);
	free(full_path);
	return (NULL);
}

char	**get_paths_from_env(char **env)
{
	char	*path_value;
	char	**paths;

	path_value = get_env_value(env, "PATH");
	if (!path_value)
		return (NULL);
	paths = ft_split(path_value, ':');
	return (paths);
}

static char	*handle_absolute_path(char *cmd)
{
	struct stat	statbuf;

	if (stat(cmd, &statbuf) == -1)
		return (NULL);
	if (S_ISREG(statbuf.st_mode))
		return (ft_strdup(cmd));
	return (NULL);
}

static char	*search_in_path(char *cmd, char **env)
{
	char	**paths;
	char	*cmd_path;
	int		i;

	paths = get_paths_from_env(env);
	if (!paths)
		return (NULL);
	i = 0;
	while (paths[i])
	{
		cmd_path = check_path(paths[i], cmd);
		if (cmd_path)
		{
			free_string_array(paths);
			return (cmd_path);
		}
		i++;
	}
	free_string_array(paths);
	return (NULL);
}

char	*find_command_path(char *cmd, char **env)
{
	if (!cmd || !*cmd)
		return (NULL);
	if (ft_strchr(cmd, '/'))
		return (handle_absolute_path(cmd));
	return (search_in_path(cmd, env));
}

void	print_error(char *cmd, char *msg)
{
	write(STDERR_FILENO, "minishell: ", 11);
	write(STDERR_FILENO, cmd, ft_strlen(cmd));
	write(STDERR_FILENO, ": ", 2);
	write(STDERR_FILENO, msg, ft_strlen(msg));
	write(STDERR_FILENO, "\n", 1);
}
*/
