#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define MAXLI 2048  // longeur maximale d'une ligne de commande

// cd
void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "mbash: cd: missing argument\n");
    } else if (chdir(path) != 0) {
        perror("mbash: cd");
    }
}

// prompt printer
void print_prompt() {
    char cwd[MAXLI];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s $ ", cwd);
    } else {
        printf("$ ");
    }
}


void execute(char *cmd) {
    char *args[MAXLI + 1];
    char *envp[] = {"PATH=/bin:/usr/bin", "HOME=/home/user", NULL};

    int background = 0;
    pid_t pid;


    // Recherche du caractère & dans la commande pour détecter une exécution en arrière-plan.
    char *esper = strchr(cmd, '&');
    if (esper != NULL) {  // Si trouvé, on remplace & par \0 pour que la commande soit correctement interprétée.
        background = 1;
        *esper = '\0';
    }

    // split
    int i = 0;
    char *token = strtok(cmd, " ");  // Utilisation de strtok pour diviser la commande en arguments séparés par des espaces.
    while (token != NULL) {
        // Les arguments sont stockés dans args, qui sera utilisé pour l'exécution avec execvp.
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // empty
    if (args[0] == NULL) {
        return;
    }


    /*
    MBASH PRIMAL COMMANDS
    */

    //ls
    if(strcmp(args[0], "ls") == 0) {
        struct dirent *entry;
        DIR *dir;

        // wd si pas de path
        const char *path = args[1] ? args[1] : ".";

        // ouverture du dossier
        if ((dir = opendir(path)) == NULL) {
            perror("mbash");
            return;
        }

        // lecture de chaque entrée du wd
        while ((entry = readdir(dir)) != NULL) {
            printf("%s\n", entry->d_name);
        }

        closedir(dir);

        return;
    }


    // cd
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    }

    // pwd
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAXLI];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return;
    }


    // OTHERS
    pid = fork();


    if (pid == 0) {
        if (execve(args[0], args, envp) == -1) {
            perror("mbash");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("mbash");
    } else {
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}




int main() {
    char cmd[MAXLI];

    while (1) {
        print_prompt();

        if (fgets(cmd, MAXLI, stdin) == NULL) {
            break; 
        }
        cmd[strcspn(cmd, "\n")] = '\0';


        // --> "exit" pour quitter le shell
        if (strcmp(cmd, "exit") == 0) {
            break;
        }

        execute(cmd);
    }

    return 0;
}