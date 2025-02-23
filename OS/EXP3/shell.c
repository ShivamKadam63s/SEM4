/*
 * shell.c - A simple command-line shell using fork() and execvp()
 *
 * How to compile:
 *     gcc -o shell shell.c
 *
 * How to run:
 *     ./shell
 *
 * Features:
 *   - Prompts the user with "myshell> ".
 *   - Reads user input using getline().
 *   - Tokenizes the input into command and arguments.
 *   - Exits the shell when the user types "exit".
 *   - Uses fork() to create a child process.
 *   - Uses execvp() in the child to execute external commands.
 *   - The parent process waits for the child to finish.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/wait.h>
 
 // Declaration of the built-in command dispatcher from shell_commands.c
 int execute_builtin(char **args);
 
 #define MAX_ARGS 64
 
 int main() {
     char *line = NULL;
     size_t bufsize = 0;
     
     while (1) {
         // Display prompt
         printf("myshell> ");
         
         // Read a line from standard input
         if (getline(&line, &bufsize, stdin) == -1) {
             perror("getline");
             break;
         }
         
         // Remove trailing newline character
         line[strcspn(line, "\n")] = '\0';
         
         // Skip empty input
         if (strlen(line) == 0) {
             continue;
         }
         
         // Tokenize the input line into arguments
         char *args[MAX_ARGS];
         int i = 0;
         char *token = strtok(line, " ");
         while (token != NULL && i < MAX_ARGS - 1) {
             args[i++] = token;
             token = strtok(NULL, " ");
         }
         args[i] = NULL; // Null-terminate the argument list
         
         // Check for the built-in exit command
         if (strcmp(args[0], "exit") == 0) {
             break;
         }
         
         // Check and execute built-in commands (like cd and help)
         int builtin_status = execute_builtin(args);
         if (builtin_status != -1) {
             // Built-in command executed; continue to next prompt.
             continue;
         }
         
         // Fork a new process for external commands
         pid_t pid = fork();
         if (pid < 0) {
             perror("fork failed");
             continue;
         }
         if (pid == 0) {
             // In child process: execute the external command
             if (execvp(args[0], args) == -1) {
                 perror("execvp failed");
             }
             exit(EXIT_FAILURE);
         } else {
             // In parent process: wait for child to finish
             int status;
             waitpid(pid, &status, 0);
         }
     }
     
     free(line);
     return 0;
 }
 
