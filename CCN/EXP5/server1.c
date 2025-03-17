/*
 * server.c - SMTP Server for Local Network
 *
 * Compile with:
 *    gcc -pthread server.c -o server
 *
 * Usage:
 *    ./server <domain> <port> <remote_server_ip> <remote_server_port>
 *
 * Example (System 1 - comp.com):
 *    ./server comp.com 5000 <IP_OF_SYSTEM_2> 6000
 *
 * Example (System 2 - cse.com):
 *    ./server cse.com 6000 <IP_OF_SYSTEM_1> 5000
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <dirent.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <time.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 
 #define BUFSIZE 1024
 
 // Allowed users per domain
 char *comp_users[] = {"user1@comp.com", "user2@comp.com"};
 char *cse_users[]  = {"user3@cse.com", "user4@cse.com"};
 
 // Server information
 char domain[100];
 char remote_ip[100];
 int remote_port;
 
 // Helper function to send messages
 void send_msg(int sock, const char *msg) {
     send(sock, msg, strlen(msg), 0);
 }
 
 // Helper function to receive a single line from socket
 int recv_line(int sock, char *buffer, int size) {
     int i = 0;
     char c;
     int n;
     while (i < size - 1) {
         n = recv(sock, &c, 1, 0);
         if (n <= 0) break;
         buffer[i++] = c;
         if (c == '\n') break;
     }
     buffer[i] = '\0';
     return i;
 }
 
 // Function to forward an email to the remote server
 void forward_email(const char *sender, const char *recipient, const char *message) {
     int sockfd;
     struct sockaddr_in servaddr;
 
     // Create socket
     if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         perror("Socket creation failed");
         return;
     }
 
     servaddr.sin_family = AF_INET;
     servaddr.sin_port = htons(remote_port);
     inet_pton(AF_INET, remote_ip, &servaddr.sin_addr);
 
     // Connect to the remote SMTP server
     if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
         perror("Failed to connect to remote server");
         close(sockfd);
         return;
     }
 
     // Send mail forwarding request
     char forward_data[BUFSIZE];
     snprintf(forward_data, sizeof(forward_data), "FORWARD\nFROM:%s\nTO:%s\nMESSAGE:%s\nEND\n", sender, recipient, message);
     send_msg(sockfd, forward_data);
     
     close(sockfd);
 }
 
 // Handle client session
 void handle_client(int client_sock) {
     char buffer[BUFSIZE];
     char username[100];
 
     // Ask for username
     send_msg(client_sock, "Enter username: ");
     recv_line(client_sock, username, BUFSIZE);
     username[strcspn(username, "\r\n")] = '\0';
 
     // Validate user
     int valid = 0;
     if (strcmp(domain, "comp.com") == 0) {
         for (int i = 0; i < 2; i++) {
             if (strcmp(username, comp_users[i]) == 0) valid = 1;
         }
     } else if (strcmp(domain, "cse.com") == 0) {
         for (int i = 0; i < 2; i++) {
             if (strcmp(username, cse_users[i]) == 0) valid = 1;
         }
     }
 
     if (!valid) {
         send_msg(client_sock, "Invalid username.\n");
         close(client_sock);
         return;
     }
 
     send_msg(client_sock, "Welcome! Type 'send <recipient>' to send mail.\n");
 
     while (1) {
         send_msg(client_sock, "> ");
         recv_line(client_sock, buffer, BUFSIZE);
         buffer[strcspn(buffer, "\r\n")] = '\0';
 
         // Handle send command
         if (strncmp(buffer, "send ", 5) == 0) {
             char recipient[100];
             sscanf(buffer, "send %s", recipient);
 
             send_msg(client_sock, "Enter message: ");
             recv_line(client_sock, buffer, BUFSIZE);
 
             // Determine if recipient is local or remote
             char *at = strchr(recipient, '@');
             if (at && strcmp(at + 1, domain) != 0) {
                 forward_email(username, recipient, buffer);
                 send_msg(client_sock, "Message forwarded to the remote server.\n");
             } else {
                 // Store locally
                 char filename[256];
                 snprintf(filename, sizeof(filename), "mailbox/%s_to_%s.txt", username, recipient);
                 FILE *fp = fopen(filename, "w");
                 if (fp) {
                     fprintf(fp, "FROM: %s\nTO: %s\nMESSAGE: %s\n", username, recipient, buffer);
                     fclose(fp);
                     send_msg(client_sock, "Message sent.\n");
                 }
             }
         } else if (strcmp(buffer, "exit") == 0) {
             send_msg(client_sock, "Goodbye!\n");
             break;
         } else {
             send_msg(client_sock, "Invalid command.\n");
         }
     }
     
     close(client_sock);
 }
 
 // Thread function for each client
 void *client_thread(void *arg) {
     int client_sock = *((int *)arg);
     handle_client(client_sock);
     free(arg);
     return NULL;
 }
 
 int main(int argc, char *argv[]) {
     if (argc != 5) {
         fprintf(stderr, "Usage: %s <domain> <port> <remote_server_ip> <remote_server_port>\n", argv[0]);
         exit(1);
     }
 
     strcpy(domain, argv[1]);
     int port = atoi(argv[2]);
     strcpy(remote_ip, argv[3]);
     remote_port = atoi(argv[4]);
 
     mkdir("mailbox", 0700);
 
     int sockfd, client_sock;
     struct sockaddr_in servaddr, cliaddr;
     socklen_t cli_len = sizeof(cliaddr);
 
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     servaddr.sin_family = AF_INET;
     servaddr.sin_addr.s_addr = INADDR_ANY;
     servaddr.sin_port = htons(port);
 
     bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
     listen(sockfd, 5);
 
     printf("Server (%s) listening on port %d\n", domain, port);
 
     while (1) {
         client_sock = accept(sockfd, (struct sockaddr *)&cliaddr, &cli_len);
         int *pclient = malloc(sizeof(int));
         *pclient = client_sock;
         pthread_t tid;
         pthread_create(&tid, NULL, client_thread, pclient);
         pthread_detach(tid);
     }
 
     return 0;
 }
 