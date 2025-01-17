#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>

#define MAXLI 2048

extern char **environ;

bool info = true;

/*
 * Commande pour changer de dossier
 */
void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "mbash: cd: argument manquant\n");
    } else if (chdir(path) != 0) {
        perror("mbash: cd");
    } else if (info) {
        char cwd[MAXLI];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("[INFO] Dossier changé en %s\n", cwd);
        } else {
            perror("mbash: cd info getcwd");
        }
    }
}

/*
 * Liste les fichiers (mode simple ou détaillé selon `-l`)
 */
void list_directory(const char *path, bool detailed) {
    struct dirent *entry;
    DIR *dir;
    struct stat file_stat;

    if ((dir = opendir(path)) == NULL) {
        perror("mbash");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (detailed) {
            char full_path[MAXLI];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            if (stat(full_path, &file_stat) == -1) {
                perror("mbash: stat");
                continue;
            }

            // Permissions
            printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
            printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
            printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
            printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
            printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
            printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
            printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
            printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
            printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

            // Liens, taille et date
            printf(" %ld", file_stat.st_nlink);
            printf(" %ld", file_stat.st_size);

            char time_buf[80];
            strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&file_stat.st_mtime));
            printf(" %s", time_buf);
        }

        printf(" %s\n", entry->d_name);
    }

    closedir(dir);
}

/*
 * Commande pour afficher le dossier courant
 */
void print_working_directory() {
    char cwd[MAXLI];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("mbash");
    }
}

/*
 * Execute une commande avec recherche dans le PATH
 */
void execute_with_execve(char *cmd) {
    char *args[MAXLI + 1];
    char *path_env = getenv("PATH");
    char *path_dirs[MAXLI];
    char full_path[MAXLI];
    int background = 0;
    pid_t pid;

    // Découper la commande en arguments
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // Vérifie si la commande doit s'exécuter en arrière-plan
    if (args[i - 1] && strcmp(args[i - 1], "&") == 0) {
        background = 1;
        args[--i] = NULL;
    }

    if (args[0] == NULL) return;

    // Commandes internes
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    } else if (strcmp(args[0], "pwd") == 0) {
        print_working_directory();
        return;
    } else if (strcmp(args[0], "info") == 0) {
        info = !info;
        printf("[INFO] Mode info %s\n", info ? "ON" : "OFF");
        return;
    } else if (strcmp(args[0], "help") == 0) {
        printf("Commandes internes :\n");
        printf("cd [path] : Changer de dossier\n");
        printf("pwd : Affiche le dossier courant\n");
        printf("info : Active/Désactive le mode info\n");
        printf("help : Affiche ce message\n");
        return;
    } else if (strcmp(args[0], "ls") == 0) {
        const char *path = ".";
        bool detailed = false;

        // Analyse des options pour `ls`
        for (int j = 1; args[j] != NULL; j++) {
            if (strcmp(args[j], "-l") == 0) {
                detailed = true;
            } else {
                path = args[j];
            }
        }

        list_directory(path, detailed);
        return;
    }

    // Exécution des commandes externes
    pid = fork();
    if (pid == 0) {
        // Processus enfant : recherche dans le PATH
        for (path = strtok(path_env, ":"); path; path = strtok(NULL, ":")) {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, args[0]);
            execve(full_path, args, environ);
        }
        perror("mbash");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("mbash: fork");
    } else {
        if (!background) waitpid(pid, NULL, 0);
    }
}

/*
 * Fonction principale
 */
int main() {
    char cmd[MAXLI];

    while (1) {
        printf("mbash> ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        cmd[strcspn(cmd, "\n")] = '\0';  // Enlève le saut de ligne
        if (strcmp(cmd, "exit") == 0) break;
        execute_with_execve(cmd);
    }

    return 0;
}
