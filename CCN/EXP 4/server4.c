#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define PORT 5003
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define USERNAME_LEN 32

// Structure to hold client information including IP.
typedef struct {
    int socket;
    char username[USERNAME_LEN];
    char ip[INET_ADDRSTRLEN]; // Stores client IP address
    int active;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t lock;

// Broadcasts a message to all active clients except the one with socket 'exclude_sock'
void broadcast_message(const char *message, int exclude_sock) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != exclude_sock) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

// Sends a direct (private) message from 'sender' to 'recipient'
void direct_message(const char *sender, const char *recipient, const char *message) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, recipient) == 0) {
            char dm_buffer[BUFFER_SIZE + 50];
            snprintf(dm_buffer, sizeof(dm_buffer), "[DM from %s]: %s\n", sender, message);
            send(clients[i].socket, dm_buffer, strlen(dm_buffer), 0);
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

// Closes and marks a client slot as inactive.
void remove_client(int index) {
    pthread_mutex_lock(&lock);
    clients[index].active = 0;
    close(clients[index].socket);
    pthread_mutex_unlock(&lock);
}

// Sends the list of currently connected usernames (excluding the new client) to the given client.
void send_user_list(int client_socket, int self_index) {
    char list_buffer[BUFFER_SIZE];
    memset(list_buffer, 0, BUFFER_SIZE);
    strcat(list_buffer, "Currently connected users: [");
    int first = 1;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && i != self_index) {
            if (!first) {
                strcat(list_buffer, ", ");
            }
            strcat(list_buffer, clients[i].username);
            first = 0;
        }
    }
    pthread_mutex_unlock(&lock);
    if (first) {
        strcat(list_buffer, "None");
    }
    strcat(list_buffer, "]\n");
    send(client_socket, list_buffer, strlen(list_buffer), 0);
}

// Thread function to handle a connected client.
void *client_handler(void *arg) {
    int index = *(int *)arg;
    free(arg);
    
    int sock = clients[index].socket;
    char username[USERNAME_LEN];
    strcpy(username, clients[index].username);
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE + 50];
    
    // Send the list of currently connected users to this new client.
    send_user_list(sock, index);
    
    // Send welcome message.
    char welcome_msg[BUFFER_SIZE];
    snprintf(welcome_msg, sizeof(welcome_msg),
             "Welcome %s! Type '@username message' for a direct message, or just type to broadcast.\n",
             username);
    send(sock, welcome_msg, strlen(welcome_msg), 0);
    
    // Notify others that a new user has joined.
    snprintf(msg, sizeof(msg), "%s has joined the chat.\n", username);
    broadcast_message(msg, sock);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0) {
            pthread_mutex_lock(&lock);
            printf("Client %s (IP: %s) disconnected.\n", username, clients[index].ip);
            pthread_mutex_unlock(&lock);
            remove_client(index);
            snprintf(msg, sizeof(msg), "%s has left the chat.\n", username);
            broadcast_message(msg, -1);
            break;
        }
        buffer[valread] = '\0';
        
        // Check if client wants to exit.
        if (strncmp(buffer, "exit", 4) == 0) {
            pthread_mutex_lock(&lock);
            printf("Client %s (IP: %s) exited.\n", username, clients[index].ip);
            pthread_mutex_unlock(&lock);
            remove_client(index);
            snprintf(msg, sizeof(msg), "%s has left the chat.\n", username);
            broadcast_message(msg, -1);
            break;
        }
        
        // Check for direct message pattern.
        if (buffer[0] == '@') {
            char *space_ptr = strchr(buffer, ' ');
            if (space_ptr != NULL) {
                *space_ptr = '\0';
                char *recipient = buffer + 1;  // Skip '@'
                char *dm_message = space_ptr + 1;
                direct_message(username, recipient, dm_message);
            }
        } else {
            // Regular broadcast.
            snprintf(msg, sizeof(msg), "%s: %s", username, buffer);
            broadcast_message(msg, sock);
        }
    }
    return NULL;
}

// Signal handler to gracefully shut down the server.
void handle_signal(int sig) {
    printf("\nServer shutting down. Closing all connections.\n");
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            send(clients[i].socket, "Server is shutting down. Chat will end.\n", 43, 0);
            close(clients[i].socket);
            clients[i].active = 0;
        }
    }
    pthread_mutex_unlock(&lock);
    exit(0);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    
    pthread_mutex_init(&lock, NULL);
    
    // Initialize client slots.
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].socket = -1;
        memset(clients[i].username, 0, USERNAME_LEN);
        memset(clients[i].ip, 0, INET_ADDRSTRLEN);
    }
    
    // Setup signal handler for Ctrl-C.
    signal(SIGINT, handle_signal);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Group/Direct Chat Server (Part‑4) listening on port %d...\n", PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_socket < 0) {
            perror("accept failed");
            continue;
        }
        // Obtain the client's IP address.
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        
        // Prompt for the client's username.
        char prompt[] = "Enter your name: ";
        send(new_socket, prompt, strlen(prompt), 0);
        
        char name_buf[USERNAME_LEN];
        memset(name_buf, 0, USERNAME_LEN);
        ssize_t name_read = recv(new_socket, name_buf, USERNAME_LEN - 1, 0);
        if (name_read <= 0) {
            close(new_socket);
            continue;
        }
        name_buf[name_read] = '\0';
        char *nl = strchr(name_buf, '\n');
        if (nl) *nl = '\0';
        
        // Check for duplicate username.
        pthread_mutex_lock(&lock);
        int taken = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && strcmp(clients[i].username, name_buf) == 0) {
                taken = 1;
                break;
            }
        }
        pthread_mutex_unlock(&lock);
        
        if (taken) {
            char err_msg[] = "Name already taken. Disconnecting.\n";
            send(new_socket, err_msg, strlen(err_msg), 0);
            close(new_socket);
            continue;
        }
        
        // Find an empty slot.
        pthread_mutex_lock(&lock);
        int index = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                index = i;
                clients[i].active = 1;
                clients[i].socket = new_socket;
                strncpy(clients[i].username, name_buf, USERNAME_LEN - 1);
                strncpy(clients[i].ip, ip_str, INET_ADDRSTRLEN - 1);
                break;
            }
        }
        pthread_mutex_unlock(&lock);
        
        if (index == -1) {
            char err_msg[] = "Server full. Disconnecting.\n";
            send(new_socket, err_msg, strlen(err_msg), 0);
            close(new_socket);
            continue;
        }
        
        printf("Client %s connected from IP %s (Socket %d).\n", name_buf, ip_str, new_socket);
        
        pthread_t tid;
        int *pindex = malloc(sizeof(int));
        *pindex = index;
        pthread_create(&tid, NULL, client_handler, pindex);
        pthread_detach(tid);
    }
    
    close(server_fd);
    pthread_mutex_destroy(&lock);
    return 0;
}
