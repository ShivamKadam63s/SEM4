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
// Structure to pass client info to the thread
typedef struct {
    int sock;
    char domain[100];
} client_info;
// Helper function to send a string message
void send_msg(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}
// Helper function to receive one line (up to newline) from socket
int recv_line(int sock, char *buffer, int size) {
    int i = 0;
    char c;
    int n;
    while(i < size - 1) {
        n = recv(sock, &c, 1, 0);
        if(n <= 0)
            break;
        buffer[i++] = c;
        if(c == '\n')
            break;
    }
    buffer[i] = '\0';
    return i;
}
// Show command instructions to the client (only once)
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
// Handle one client session
void handle_client_session(int client_sock, char *domain) {
    char buffer[BUFSIZE];
    char username[100];
    int i, n;
    // Ask for username
    send_msg(client_sock, "Enter username: ");
    n = recv_line(client_sock, buffer, BUFSIZE);
    if(n <= 0)
        return;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    strcpy(username, buffer);
    // Validate username based on domain
    int valid = 0;
    if(strcmp(domain, "comp.com") == 0) {
        for(i = 0; i < 2; i++) {
            if(strcmp(username, comp_users[i]) == 0) {
                valid = 1;
                break;
            }
        }
    } else if(strcmp(domain, "cse.com") == 0) {
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
    // Show command instructions once at the beginning
    show_instructions_once(client_sock);
    // Command loop (instructions not reprinted after each command)
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        n = recv_line(client_sock, buffer, BUFSIZE);
        if(n <= 0)
            break;
        buffer[strcspn(buffer, "\r\n")] = '\0';
        // Process commands
        if(strncmp(buffer, "send ", 5) == 0) {
            // "send <recipient_username>"
            char recipient[100];
            if(sscanf(buffer, "send %s", recipient) != 1) {
                send_msg(client_sock, "Invalid send command format. Use: send <recipient_username>\n");
                continue;
            }
            // Validate recipient among all allowed users
            int valid_recipient = 0;
            for(i = 0; i < 2; i++) {
                if(strcmp(recipient, comp_users[i]) == 0)
                    valid_recipient = 1;
                if(strcmp(recipient, cse_users[i]) == 0)
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
            char message[5000];
            strcpy(message, buffer);
            // Create a filename using timestamp
            time_t now = time(NULL);
            char timestr[64];
            strftime(timestr, sizeof(timestr), "%Y%m%d%H%M%S", localtime(&now));
            char filename[256];
            snprintf(filename, sizeof(filename), "mailbox/%s_to_%s_%s.txt", username, recipient, timestr);
            FILE *fp = fopen(filename, "w");
            if(fp == NULL) {
                send_msg(client_sock, "Error writing mail file.\n");
                continue;
            }
            fprintf(fp, "FROM: %s\nTO: %s\nDATE: %s", username, recipient, ctime(&now));
            fprintf(fp, "STATUS: UNREAD\n\n");
            fprintf(fp, "%s", message);
            fclose(fp);
            send_msg(client_sock, "Message successfully sent.\n");
        }
        else if(strcmp(buffer, "read inbox") == 0) {
            // List inbox messages for the user
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
                    // Check if filename contains _to_<username>_ (i.e. user is recipient)
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
                        // Update status from UNREAD to READ if needed
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
        else if(strcmp(buffer, "delete") == 0) {
            // List inbox messages for deletion
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
        else if(strcmp(buffer, "view sent") == 0) {
            // List sent messages (files starting with the sender's username)
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
        else if(strcmp(buffer, "exit") == 0) {
            send_msg(client_sock, "Goodbye!\n");
            break;
        }
        else {
            send_msg(client_sock, "Invalid command. Please follow the instructions.\n");
        }
    }
}
// Thread function to handle each client connection
void *client_thread(void *arg) {
    client_info *cinfo = (client_info *)arg;
    handle_client_session(cinfo->sock, cinfo->domain);
    close(cinfo->sock);
    free(cinfo);
    return NULL;
}
int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <domain> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *domain = argv[1];
    int port = atoi(argv[2]);
    // Create mailbox directory if not existing
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
        client_info *cinfo = malloc(sizeof(client_info));
        cinfo->sock = client_sock;
        strncpy(cinfo->domain, domain, sizeof(cinfo->domain) - 1);
        cinfo->domain[sizeof(cinfo->domain) - 1] = '\0';
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cinfo);
        pthread_detach(tid);
    }
    close(sockfd);
    return 0;
}
