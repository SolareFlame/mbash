#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>

#define MAXLI 2048  // longeur maximale d'une ligne de commande

/*
COMMANDS FUNCTIONS
*/

// cd
void change_directory(char *path) {
    if (path == NULL) {
        fprintf(stderr, "mbash: cd: missing argument\n");
    } else if (chdir(path) != 0) {
        perror("mbash: cd");
    }
}

//ls
void list_directory(const char *path) {
    struct dirent *entry;
    DIR *dir;

    if ((dir = opendir(path)) == NULL) {
        perror("mbash");
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
        perror("mbash");
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
        const char *path = args[1] != NULL ? args[1] : ".";
        list_directory(path);
        return;
    }

    // cd
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
        return;
    }

    // pwd
    if (strcmp(args[0], "pwd") == 0) {
        print_working_directory();
        printf("\n");
        return;
    }

    /*
    /BIN/ COMMANDS CALL
    */
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
        print_working_directory();
        printf(" § ");

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