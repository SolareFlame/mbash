#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXLI 2048  // Longueur maximale d'une ligne de commande
bool info = true;

// Fonction pour changer de répertoire
void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "mbash: cd: missing argument\n");
    } else if (chdir(path) != 0) {
        perror("mbash: cd");
    } else if (info) {
        char cwd[MAXLI];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("[INFO] Répertoire changé : %s\n", cwd);
        } else {
            perror("mbash: cd");
        }
    }
}

// Fonction pour afficher les fichiers
void list_directory(const char *path, bool detailed) {
    struct dirent *entry;
    DIR *dir;
    if ((dir = opendir(path)) == NULL) {
        perror("mbash");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (detailed) {
            printf("%s\n", entry->d_name);  // Affichage vertical
        } else {
            printf("%s ", entry->d_name);  // Affichage horizontal
        }
    }
    if (!detailed) {
        printf("\n");  // Ligne vide pour l'affichage horizontal
    }
    closedir(dir);
}

// Fonction pour exécuter une commande
void execute(char *cmd) {
    char *args[MAXLI + 1];
    char *envp[] = {"PATH=/bin:/usr/bin", NULL};
    int background = 0;

    // Gestion des commandes en arrière-plan
    char *esper = strchr(cmd, '&');
    if (esper != NULL) {
        background = 1;
        *esper = '\0';
    }

    // Découpage de la commande en arguments
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // Si aucune commande n'est entrée
    if (args[0] == NULL) {
        return;
    }

    // Commandes internes
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    }
    if (strcmp(args[0], "ls") == 0) {
        bool detailed = (args[1] != NULL && strcmp(args[1], "-l") == 0);
        list_directory(args[1] ? args[1] : ".", detailed);
        return;
    }
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAXLI];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("mbash: pwd");
        }
        return;
    }
    if (strcmp(args[0], "info") == 0) {
        info = !info;
        printf("[INFO] Mode info %s\n", info ? "activé" : "désactivé");
        return;
    }
    if (strcmp(args[0], "help") == 0) {
        printf("mbash: commandes disponibles :\n");
        printf("  cd [path] : changer de répertoire\n");
        printf("  ls [-l] [path] : lister les fichiers\n");
        printf("  pwd : afficher le répertoire courant\n");
        printf("  info : activer/désactiver le mode info\n");
        printf("  exit : quitter le shell\n");
        return;
    }

    // Commandes externes
    pid_t pid = fork();
    if (pid == 0) {
        execve(args[0], args, envp);
        perror("mbash: execve");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("mbash: fork");
    } else {
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}

// Fonction principale
int main() {
    char *cmd;
    char prompt[MAXLI];

    using_history();  // Initialisation de l'historique

    while (1) {
        // Affichage du prompt avec le répertoire courant
        char cwd[MAXLI];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(prompt, sizeof(prompt), "%s mbash> ", cwd);
        } else {
            strcpy(prompt, "mbash> ");
        }

        // Lecture de la commande avec historique
        cmd = readline(prompt);
        if (!cmd) {
            break;  // Ctrl+D pour quitter
        }

        // Ajout à l'historique si non vide
        if (*cmd) {
            add_history(cmd);
        }

        // Gestion de la commande "exit"
        if (strcmp(cmd, "exit") == 0) {
            free(cmd);
            break;
        }

        // Exécution de la commande
        execute(cmd);
        free(cmd);
    }

    return 0;
}
