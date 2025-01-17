#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>

#define MAXLI 2048  // Longueur maximale d'une ligne de commande
#define HIST_SIZE 100

bool info = true;
char *history[HIST_SIZE];  // Tableau pour stocker l'historique des commandes
int history_index = 0;
int history_pos = 0;

// Fonction pour changer de répertoire
void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "mbash: cd: missing argument\n");
    } else {
        // Vérifie si le répertoire existe
        if (chdir(path) != 0) {
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

// Fonction pour configurer le terminal pour lire les touches sans attendre un \n
void set_input_mode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);  // Mode non canonique, pas d'écho
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Fonction pour lire une touche sans attendre un retour chariot
int get_key() {
    int ch = getchar();
    return ch;
}

// Fonction pour récupérer l'entrée avec gestion des flèches et historique
void get_input(char *cmd) {
    int ch;
    int index = 0;
    int max_len = MAXLI - 1;
    while ((ch = get_key()) != '\n' && ch != EOF) {
        if (ch == 27) {  // Détection des séquences d'échappement (flèches)
            ch = get_key();  // Récupérer la seconde touche (flèche gauche, droite, haut, bas)
            if (ch == 91) {
                ch = get_key();  // Code pour flèche
                if (ch == 65) {  // Flèche haut
                    if (history_pos > 0) {
                        history_pos--;
                        strcpy(cmd, history[history_pos]);
                        printf("\r%s", cmd);
                        fflush(stdout);
                    }
                } else if (ch == 66) {  // Flèche bas
                    if (history_pos < history_index - 1) {
                        history_pos++;
                        strcpy(cmd, history[history_pos]);
                        printf("\r%s", cmd);
                        fflush(stdout);
                    } else {
                        memset(cmd, 0, sizeof(cmd));
                        printf("\r");
                    }
                }
            }
        } else if (ch == 127) {  // Backspace
            if (index > 0) {
                cmd[--index] = '\0';
                printf("\r%s", cmd);
                fflush(stdout);
            }
        } else if (index < max_len) {
            cmd[index++] = ch;
            cmd[index] = '\0';
            printf("%c", ch);
            fflush(stdout);
        }
    }
    cmd[index] = '\0';
    printf("\n");  // Retour à la ligne après la commande
}

// Fonction principale
int main() {
    char cmd[MAXLI];
    char prompt[MAXLI];

    // Initialisation du terminal
    set_input_mode();

    while (1) {
        // Affichage du prompt avec le répertoire courant
        char cwd[MAXLI];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(prompt, sizeof(prompt), "%s mbash> ", cwd);
        } else {
            strcpy(prompt, "mbash> ");
        }

        // Affichage du prompt
        printf("%s", prompt);
        fflush(stdout);

        // Récupérer l'entrée utilisateur avec gestion des flèches
        get_input(cmd);

        // Gestion de la commande "exit"
        if (strcmp(cmd, "exit") == 0) {
            break;
        }

        // Ajouter la commande à l'historique
        if (history_index < HIST_SIZE) {
            history[history_index] = strdup(cmd);
            history_index++;
            history_pos = history_index;
        }

        // Exécution de la commande
        execute(cmd);
    }

    // Libérer l'historique
    for (int i = 0; i < history_index; i++) {
        free(history[i]);
    }

    return 0;
}
