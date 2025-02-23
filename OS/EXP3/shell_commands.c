/*
 * shell_commands.c - Implementation of built-in commands for the shell
 *
 * This file contains implementations for commands that need to be handled
 * directly by the shell (e.g., cd, help). Since commands like 'cd' affect
 * the shell's internal state, they cannot be run as external programs.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 
 // Built-in command: cd
 int shell_cd(char **args) {
     if (args[1] == NULL) {
         fprintf(stderr, "cd: expected argument\n");
         return 1;
     }
     if (chdir(args[1]) != 0) {
         perror("cd failed");
     }
     return 1;
 }
 
 // Built-in command: help
 int shell_help(char **args) {
     printf("Shell built-in commands:\n");
     printf("  cd [dir]    : Change the current directory to [dir]\n");
     printf("  help        : Display this help message\n");
     printf("  exit        : Exit the shell\n");
     return 1;
 }
 
 /*
  * execute_builtin:
  *   Checks if the command entered is a built-in command.
  *   If yes, executes it and returns a non-negative value.
  *   Returns -1 if the command is not a built-in command.
  */
 int execute_builtin(char **args) {
     if (args[0] == NULL) {
         // No command entered
         return 0;
     }
     
     if (strcmp(args[0], "cd") == 0) {
         return shell_cd(args);
     }
     
     if (strcmp(args[0], "help") == 0) {
         return shell_help(args);
     }
     
     // Add additional built-in commands here if needed
     
     // Return -1 to indicate the command is not built-in
     return -1;
 }
 
