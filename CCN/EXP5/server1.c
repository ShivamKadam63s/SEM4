/*
 * Combined SMTP Server with Local Mailbox and Forwarding Functionality
 *
 * Compile with:
 *    gcc -pthread server.c -o server
 *
 * Usage:
 *    ./server <domain> <port> <remote_server_ip> <remote_server_port>
 *
 * Examples:
 *   On System 1 (comp.com):
 *       ./server comp.com 5000 <IP_OF_SYSTEM_2> 6000
 *
 *   On System 2 (cse.com):
 *       ./server cse.com 6000 <IP_OF_SYSTEM_1> 5000
 *
 * This server supports:
 *   - Interactive client commands:
 *         send <recipient_username>
 *         read inbox
 *         delete
 *         view sent
 *         exit
 *   - If the recipient’s domain (after '@') does not match the local domain,
 *     the email is forwarded to the remote server.
 *   - When a connection comes from a forwarding server (the message starts with
 *     "FORWARD"), it is handled separately and stored in the recipient’s mailbox.
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
#include <errno.h>

#define BUFSIZE 1024

// Allowed users for each domain
char *comp_users[] = {"user1@comp.com", "user2@comp.com"};
char *cse_users[]  = {"user3@cse.com", "user4@cse.com"};

// Global variables for local domain and remote server configuration
char domain[100];
char remote_ip[100];
int remote_port;

// Helper function to send a message to a socket
void send_msg(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// Helper function to receive one line (up to newline) from a socket
int recv_line(int sock, char *buffer, int size) {
    int i = 0;
    char c;
    int n;
    while(i < size - 1) {
        n = recv(sock, &c, 1, 0);
        if(n <= 0) break;
        buffer[i++] = c;
        if(c == '\n') break;
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
    
    // Connect to the remote server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Failed to connect to remote server");
        close(sockfd);
        return;
    }
    
    // Compose the forwarding message
    char forward_data[BUFSIZE];
    snprintf(forward_data, sizeof(forward_data),
             "FORWARD\nFROM:%s\nTO:%s\nMESSAGE:%s\nEND\n",
             sender, recipient, message);
    send_msg(sockfd, forward_data);
    
    close(sockfd);
}

// Handle an incoming forwarded email from a remote server.
// The connection will send lines: FORWARD, FROM:, TO:, MESSAGE:, ... , END
void handle_forwarded_email(int sock) {
    char buffer[BUFSIZE];
    // "FORWARD" line already consumed by caller.
    // Read FROM:
    recv_line(sock, buffer, BUFSIZE);
    char sender[100] = "";
    if (strncmp(buffer, "FROM:", 5) == 0) {
        strcpy(sender, buffer + 5);
        sender[strcspn(sender, "\r\n")] = '\0';
    }
    // Read TO:
    recv_line(sock, buffer, BUFSIZE);
    char recipient[100] = "";
    if (strncmp(buffer, "TO:", 3) == 0) {
        strcpy(recipient, buffer + 3);
        recipient[strcspn(recipient, "\r\n")] = '\0';
    }
    // Read MESSAGE:
    recv_line(sock, buffer, BUFSIZE);
    char message[5000] = "";
    if (strncmp(buffer, "MESSAGE:", 8) == 0) {
        strcpy(message, buffer + 8);
        message[strcspn(message, "\r\n")] = '\0';
    }
    // Consume lines until "END" is encountered.
    while(1) {
        recv_line(sock, buffer, BUFSIZE);
        if(strncmp(buffer, "END", 3) == 0)
            break;
    }
    
    // Save the forwarded email in the mailbox directory.
    time_t now = time(NULL);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y%m%d%H%M%S", localtime(&now));
    char filename[256];
    // Filename contains recipient so that "read inbox" (which searches for _to_<username>_) works.
    snprintf(filename, sizeof(filename), "mailbox/forwarded_%s_to_%s_%s.txt", sender, recipient, timestr);
    
    FILE *fp = fopen(filename, "w");
    if(fp) {
        fprintf(fp, "FROM: %s\nTO: %s\nDATE: %s", sender, recipient, ctime(&now));
        fprintf(fp, "STATUS: UNREAD\n\n");
        fprintf(fp, "%s", message);
        fclose(fp);
    }
    send_msg(sock, "Forwarded email received and stored.\n");
    close(sock);
}

// Show command instructions once the client logs in.
void show_instructions_once(int sock) {
    char instr[1024];
    snprintf(instr, sizeof(instr),
        "\n*** Command Format ***\n"
        "1. send <recipient_username>   : To send a message (message is one line).\n"
        "2. read inbox                  : To read your inbox. You will be prompted to enter message number (-1 to cancel).\n"
        "3. delete                      : To delete a message from your inbox.\n"
        "4. view sent                   : To view your sent messages.\n"
        "5. exit                       : To exit the session.\n"
        "Enter your command:\n");
    send_msg(sock, instr);
}

// Handle an interactive client session (for a connected user)
void handle_client_session(int client_sock, char *domain_local) {
    char buffer[BUFSIZE];
    char username[100];
    int n, i;
    
    // Ask for username
    send_msg(client_sock, "Enter username: ");
    n = recv_line(client_sock, buffer, BUFSIZE);
    if(n <= 0) return;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    strcpy(username, buffer);
    
    // Validate username based on domain
    int valid = 0;
    if(strcmp(domain_local, "comp.com") == 0) {
        for(i = 0; i < 2; i++) {
            if(strcmp(username, comp_users[i]) == 0) {
                valid = 1;
                break;
            }
        }
    } else if(strcmp(domain_local, "cse.com") == 0) {
        for(i = 0; i < 2; i++) {
            if(strcmp(username, cse_users[i]) == 0) {
                valid = 1;
                break;
            }
        }
    }
    if(!valid) {
        send_msg(client_sock, "Invalid username. Connection closing.\n");
        return;
    }
    char welcome[256];
    snprintf(welcome, sizeof(welcome), "Welcome %s!\n", username);
    send_msg(client_sock, welcome);
    
    // Show available commands
    show_instructions_once(client_sock);
    
    // Main command loop
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        n = recv_line(client_sock, buffer, BUFSIZE);
        if(n <= 0)
            break;
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        // Process "send" command
        if(strncmp(buffer, "send ", 5) == 0) {
            char recipient[100];
            if(sscanf(buffer, "send %s", recipient) != 1) {
                send_msg(client_sock, "Invalid send command format. Use: send <recipient_username>\n");
                continue;
            }
            // Validate that recipient is an allowed user
            int valid_recipient = 0;
            for(i = 0; i < 2; i++) {
                if(strcmp(recipient, comp_users[i]) == 0 || strcmp(recipient, cse_users[i]) == 0)
                    valid_recipient = 1;
            }
            if(!valid_recipient) {
                send_msg(client_sock, "Invalid recipient username.\n");
                continue;
            }
            send_msg(client_sock, "Enter your message (one line): ");
            memset(buffer, 0, sizeof(buffer));
            n = recv_line(client_sock, buffer, BUFSIZE);
            if(n <= 0)
                break;
            buffer[strcspn(buffer, "\r\n")] = '\0';
            
            // Create a filename using timestamp for the sent message record
            time_t now = time(NULL);
            char timestr[64];
            strftime(timestr, sizeof(timestr), "%Y%m%d%H%M%S", localtime(&now));
            char filename[256];
            snprintf(filename, sizeof(filename), "mailbox/%s_to_%s_%s.txt", username, recipient, timestr);
            
            // Check whether the recipient is local or remote (by comparing domain part)
            char *at_sign = strchr(recipient, '@');
            if(at_sign && strcmp(at_sign + 1, domain_local) != 0) {
                // Forward to remote server
                forward_email(username, recipient, buffer);
                send_msg(client_sock, "Message forwarded to the remote server.\n");
                // Optionally store a sent record locally
                FILE *fp = fopen(filename, "w");
                if(fp) {
                    fprintf(fp, "FROM: %s\nTO: %s\nDATE: %s", username, recipient, ctime(&now));
                    fprintf(fp, "STATUS: SENT (FORWARDED)\n\n");
                    fprintf(fp, "%s", buffer);
                    fclose(fp);
                }
            } else {
                // Local delivery: store the message in the mailbox (for recipient's inbox)
                FILE *fp = fopen(filename, "w");
                if(fp) {
                    fprintf(fp, "FROM: %s\nTO: %s\nDATE: %s", username, recipient, ctime(&now));
                    fprintf(fp, "STATUS: UNREAD\n\n");
                    fprintf(fp, "%s", buffer);
                    fclose(fp);
                    send_msg(client_sock, "Message successfully sent.\n");
                } else {
                    send_msg(client_sock, "Error writing mail file.\n");
                }
            }
        }
        // Process "read inbox" command
        else if(strcmp(buffer, "read inbox") == 0) {
            DIR *d;
            struct dirent *dir;
            d = opendir("mailbox");
            if(d == NULL) {
                send_msg(client_sock, "Error opening mailbox directory.\n");
                continue;
            }
            char list[5000] = "";
            int count = 0;
            char files[100][256];
            while((dir = readdir(d)) != NULL) {
                if(dir->d_type == DT_REG) {
                    // If the file name contains _to_<username>_, it is assumed to be addressed to the user.
                    char pattern[150];
                    snprintf(pattern, sizeof(pattern), "_to_%s_", username);
                    if(strstr(dir->d_name, pattern) != NULL) {
                        strcpy(files[count], dir->d_name);
                        char entry[300];
                        snprintf(entry, sizeof(entry), "Message %d: %s\n", count+1, dir->d_name);
                        strcat(list, entry);
                        count++;
                    }
                }
            }
            closedir(d);
            if(count == 0) {
                send_msg(client_sock, "No messages in inbox.\n");
            } else {
                send_msg(client_sock, list);
                send_msg(client_sock, "Enter message number to read (or -1 to cancel): ");
                memset(buffer, 0, sizeof(buffer));
                n = recv_line(client_sock, buffer, BUFSIZE);
                if(n <= 0)
                    break;
                int msgnum = atoi(buffer);
                if(msgnum == -1) {
                    send_msg(client_sock, "Canceled reading inbox.\n");
                } else if(msgnum < 1 || msgnum > count) {
                    send_msg(client_sock, "Invalid message number.\n");
                } else {
                    char filepath[300];
                    snprintf(filepath, sizeof(filepath), "mailbox/%s", files[msgnum-1]);
                    FILE *fp = fopen(filepath, "r+");
                    if(fp == NULL) {
                        send_msg(client_sock, "Error opening message file.\n");
                    } else {
                        char fileContent[5000];
                        size_t nread = fread(fileContent, 1, sizeof(fileContent)-1, fp);
                        fileContent[nread] = '\0';
                        send_msg(client_sock, "\n----- Message Content -----\n");
                        send_msg(client_sock, fileContent);
                        send_msg(client_sock, "\n---------------------------\n");
                        // Change status from UNREAD to READ if applicable.
                        char *pos = strstr(fileContent, "STATUS: UNREAD");
                        if(pos != NULL) {
                            memcpy(pos, "STATUS: READ  ", 14);
                            fp = freopen(filepath, "w", fp);
                            if(fp != NULL)
                                fputs(fileContent, fp);
                        }
                        fclose(fp);
                    }
                }
            }
        }
        // Process "delete" command
        else if(strcmp(buffer, "delete") == 0) {
            DIR *d;
            struct dirent *dir;
            d = opendir("mailbox");
            if(d == NULL) {
                send_msg(client_sock, "Error opening mailbox directory.\n");
                continue;
            }
            char list[5000] = "";
            int count = 0;
            char files[100][256];
            while((dir = readdir(d)) != NULL) {
                if(dir->d_type == DT_REG) {
                    char pattern[150];
                    snprintf(pattern, sizeof(pattern), "_to_%s_", username);
                    if(strstr(dir->d_name, pattern) != NULL) {
                        strcpy(files[count], dir->d_name);
                        char entry[300];
                        snprintf(entry, sizeof(entry), "Message %d: %s\n", count+1, dir->d_name);
                        strcat(list, entry);
                        count++;
                    }
                }
            }
            closedir(d);
            if(count == 0) {
                send_msg(client_sock, "No messages to delete.\n");
            } else {
                send_msg(client_sock, list);
                send_msg(client_sock, "Enter message number to delete: ");
                memset(buffer, 0, sizeof(buffer));
                n = recv_line(client_sock, buffer, BUFSIZE);
                if(n <= 0)
                    break;
                int msgnum = atoi(buffer);
                if(msgnum < 1 || msgnum > count) {
                    send_msg(client_sock, "Invalid message number.\n");
                } else {
                    char filepath[300];
                    snprintf(filepath, sizeof(filepath), "mailbox/%s", files[msgnum-1]);
                    if(remove(filepath) == 0)
                        send_msg(client_sock, "Message deleted successfully.\n");
                    else
                        send_msg(client_sock, "Error deleting message.\n");
                }
            }
        }
        // Process "view sent" command
        else if(strcmp(buffer, "view sent") == 0) {
            DIR *d;
            struct dirent *dir;
            d = opendir("mailbox");
            if(d == NULL) {
                send_msg(client_sock, "Error opening mailbox directory.\n");
                continue;
            }
            char list[5000] = "";
            int count = 0;
            while((dir = readdir(d)) != NULL) {
                if(dir->d_type == DT_REG) {
                    if(strncmp(dir->d_name, username, strlen(username)) == 0) {
                        char entry[300];
                        snprintf(entry, sizeof(entry), "Message %d: %s\n", count+1, dir->d_name);
                        strcat(list, entry);
                        count++;
                    }
                }
            }
            closedir(d);
            if(count == 0)
                send_msg(client_sock, "No sent messages found.\n");
            else
                send_msg(client_sock, list);
        }
        // Process "exit" command
        else if(strcmp(buffer, "exit") == 0) {
            send_msg(client_sock, "Goodbye!\n");
            break;
        }
        else {
            send_msg(client_sock, "Invalid command. Please follow the instructions.\n");
        }
    }
}

// Thread function to handle each incoming connection
// This function first checks if the connection is a remote forward or a client session.
void *client_thread(void *arg) {
    int client_sock = *((int *)arg);
    // Peek at the first few bytes to see if the connection is for forwarded mail.
    char peek_buf[BUFSIZE];
    int bytes = recv(client_sock, peek_buf, 7, MSG_PEEK);
    if(bytes > 0 && strncmp(peek_buf, "FORWARD", 7) == 0) {
        // Consume the "FORWARD" line and handle the forwarded email.
        char temp[BUFSIZE];
        recv_line(client_sock, temp, BUFSIZE);
        handle_forwarded_email(client_sock);
        return NULL;
    }
    // Otherwise, treat as an interactive client session.
    handle_client_session(client_sock, domain);
    close(client_sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 5) {
        fprintf(stderr, "Usage: %s <domain> <port> <remote_server_ip> <remote_server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Set global configuration from command-line arguments.
    strcpy(domain, argv[1]);
    int port = atoi(argv[2]);
    strcpy(remote_ip, argv[3]);
    remote_port = atoi(argv[4]);
    
    // Create the mailbox directory if it does not exist.
    struct stat st = {0};
    if(stat("mailbox", &st) == -1) {
        if(mkdir("mailbox", 0700) != 0) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
    
    int sockfd, client_sock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cli_len = sizeof(cliaddr);
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
    
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if(listen(sockfd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server (%s) listening on port %d\n", domain, port);
    
    while(1) {
        client_sock = accept(sockfd, (struct sockaddr *)&cliaddr, &cli_len);
        if(client_sock < 0) {
            perror("accept");
            continue;
        }
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, pclient);
        pthread_detach(tid);
    }
    
    close(sockfd);
    return 0;
}
