#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAXLI 2048 

void execute_command(char *cmd) {
    char *args[MAXLI + 1];
    int background = 0;

    pid_t pid;


    // &
    char *esper = strchr(cmd, '&');
    if (esper != NULL) {
        background = 1;
        *esper = '\0';
    }

    // split
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // empty
    if (args[0] == NULL) {
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
        if (execvp(args[0], args) == -1) {
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

        execute_command(cmd);
    }

    return 0;
}