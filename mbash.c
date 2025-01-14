#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 10
#define MAX_INPUT 1024

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS + 1] = { NULL };

    printf("§ ");
    if (!fgets(input, sizeof(input), stdin)) {
        perror("Erreur de lecture");
        return EXIT_FAILURE;
    }

    int i = 0;

    char *token = strtok(input, " \t\n");
    while (token && i < MAX_ARGS) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (args[0]) {
        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("Erreur execvp");
            exit(EXIT_FAILURE);

        } else if (pid > 0) {
            wait(NULL);
        } else {
            perror("Erreur fork");
        }
    }

    return 0;
}
