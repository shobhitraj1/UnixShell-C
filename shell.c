#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

char* command_history[100];
int history_count = 0;

char* read_user_input() {
    char* input = NULL;
    size_t bufsize = 0;
    if (getline(&input, &bufsize, stdin) == -1) {
        perror("Error occured during taking input");
        exit(EXIT_FAILURE);
    }
    return input;
}

void add_to_history(char* command) {
    if (history_count < 100) {
        command_history[history_count++] = strdup(command);
        if (command_history[history_count - 1] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
    } else {
        free(command_history[0]);
        for (int i = 0; i < history_count - 1; i++) {
            command_history[i] = command_history[i + 1];
        }
        command_history[history_count - 1] = strdup(command);
        if (command_history[history_count - 1] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to print command history
void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s", i + 1, command_history[i]);
    }
}

// Function to launch a command
int launch(char* command) {
    if (strcmp(command, "exit\n") == 0) {
        return 0; // Exit the shell
    }

    if (strcmp(command, "history\n") == 0) {
        print_history();
        return 1; // Continue shell execution
    }

    // Add the command to history
    if (strcmp(command, "\n") != 0) {
        add_to_history(command);      
    }  

    pid_t pid, wpid;
    int status;
    int pipefd[2];
    int prev_pipe = 0;

    char* command_cpy;
    while ((command_cpy = strsep(&command, "|")) != NULL) {
        pipe(pipefd);
        pid = fork();

        if (pid == -1) {
            perror("Error occured during fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { //code is executed by child process

            if (prev_pipe) {
                if (dup2(prev_pipe, 0) == -1) {
                    perror("Error occured during duplicating fd");
                    exit(EXIT_FAILURE);
                }
                close(prev_pipe);
            }

            if (command != NULL) {
                if (dup2(pipefd[1], 1) == -1) {
                    perror("Error occured during duplicating fd");
                    exit(EXIT_FAILURE);
                }
            }

            char* args[1000];
            int i = 0;
            command_cpy = strtok(command_cpy, " \n");
            while (command_cpy != NULL) {
                args[i++] = command_cpy;
                command_cpy = strtok(NULL, " \n");
            }
            args[i] = NULL;

            // Execute the command
            if (execvp(args[0], args) == -1) {
                perror("execvp error");
                exit(EXIT_FAILURE);
            }
        } else { //code is executed by parent process

            if (command != NULL) {
                close(pipefd[1]);
            }

            prev_pipe = pipefd[0];

            wpid = waitpid(pid, &status, 0);
            if (wpid == -1) {
                perror("Error occured during waitpid");
                exit(EXIT_FAILURE);
            }
        }
    }

    return 1;
}

int main() {
    int status;
    do {
        printf("ta@demo:~$ ");
        char* command = read_user_input();
        status = launch(command);
        free(command);
    } while (status);

    // Free command history
    for (int i = 0; i < history_count; i++) {
        free(command_history[i]);
    }

    return 0;
}
