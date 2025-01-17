#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>

#define MAXLI 2048  // longueur maximale d'une ligne de commande
bool info = true;

/*
COMMANDS FUNCTIONS
*/

// cd
void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "mbash: cd: missing argument\n");
    } else if (chdir(path) != 0) {
        perror("mbash: cd");
    } else {
        if (info) {
            char cwd[MAXLI];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("[INFO] Directory changed to %s\n", cwd);
            } else {
                perror("mbash: cd info getcwd");
            }
        }
    }
}

// ls
void list_directory(const char *path) {
    struct dirent *entry;
    DIR *dir;

    if ((dir = opendir(path)) == NULL) {
        perror("mbash ls");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s ", entry->d_name);
    }
    printf("\n");

    closedir(dir);
}

// pwd
void print_working_directory() {
    char cwd[MAXLI];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s", cwd);
    } else {
        perror("mbash pwd");
    }
}

// Fonction pour exécuter une commande externe en utilisant execve
void execute_with_execve(char *cmd, char *args[]) {
    char *env_path = getenv("PATH");
    if (env_path == NULL) {
        fprintf(stderr, "mbash: PATH environment variable not set\n");
        exit(EXIT_FAILURE);
    }

    // Diviser PATH en répertoires
    char *path_copy = strdup(env_path); // Copie de PATH
    char *dir = strtok(path_copy, ":");
    char full_path[MAXLI];

    // Parcourir les répertoires de PATH
    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd); // Construire le chemin complet
        execve(full_path, args, environ); // Essayer d'exécuter la commande
        dir = strtok(NULL, ":");
    }

    free(path_copy); // Libérer la mémoire
}

// Fonction principale pour exécuter une commande
void execute(char *cmd) {
    char *args[MAXLI + 1];
    int background = 0;
    pid_t pid;

    // Détection de l'exécution en arrière-plan
    char *esper = strchr(cmd, '&');
    if (esper != NULL) {
        background = 1;
        *esper = '\0';
    }

    // Découper la commande en arguments
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // Si la commande est vide
    if (args[0] == NULL) {
        return;
    }

    // Commandes internes
    if (strcmp(args[0], "ls") == 0) {
        const char *path = args[1] != NULL ? args[1] : ".";
        list_directory(path);
        return;
    }
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    }
    if (strcmp(args[0], "pwd") == 0) {
        print_working_directory();
        printf("\n");
        return;
    }
    if (strcmp(args[0], "info") == 0) {
        info = !info;
        printf("[INFO] %s\n", info ? "ON" : "OFF");
        return;
    }
    if (strcmp(args[0], "help") == 0) {
        printf("mbash: a simple copy of shell\n");
        printf("cd [path] : change directory\n");
        printf("pwd : print working directory\n");
        printf("info : toggle info messages\n");
        printf("exit : exit the shell\n");
        return;
    }

    // Commandes externes
    pid = fork();
    if (pid == 0) {
        // Exécuter avec execve et gestion de PATH
        execute_with_execve(args[0], args);
        // Si execve échoue, afficher une erreur
        fprintf(stderr, "mbash: %s: command not found\n", args[0]);
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("mbash: fork");
    } else {
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}

int main() {
    char cmd[MAXLI];

    while (1) {
        print_working_directory();
        printf(" § ");
        if (fgets(cmd, MAXLI, stdin) == NULL) {
            break;
        }
        cmd[strcspn(cmd, "\n")] = '\0'; // Supprimer le \n
        if (strcmp(cmd, "exit") == 0) {
            break;
        }
        execute(cmd);
    }

    return 0;
}
