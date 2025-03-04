#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 5002
#define BUFFER_SIZE 1024

// Global arrays to hold up to 2 clients.
int client_sockets[2] = { -1, -1 };
char client_ips[2][INET_ADDRSTRLEN] = {"", ""};
pthread_t client_threads[2];
pthread_mutex_t lock;

// Helper: Sends a message to a specific client.
void send_to_client(int index, const char *message) {
    if (index >= 0 && index < 2 && client_sockets[index] != -1) {
        send(client_sockets[index], message, strlen(message), 0);
    }
}

// Helper: Notifies both connected clients.
void notify_both(const char *message) {
    for (int i = 0; i < 2; i++) {
        if (client_sockets[i] != -1) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
}

// Thread function for handling each client.
void *handle_client(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE + 50];

    // On connection, check if the other client is present.
    pthread_mutex_lock(&lock);
    int other = (idx == 0) ? 1 : 0;
    if (client_sockets[other] == -1) {
        send_to_client(idx, "Waiting for the other client to connect...\n");
    } else {
        // Both clients are connected: notify both with IP addresses.
        snprintf(msg, sizeof(msg),
                 "Both clients connected:\n  Client 1 (IP: %s)\n  Client 2 (IP: %s)\n",
                 client_ips[0], client_ips[1]);
        notify_both(msg);
    }
    pthread_mutex_unlock(&lock);

    // Main loop to receive and forward messages.
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_sockets[idx], buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            // Client disconnected.
            pthread_mutex_lock(&lock);
            printf("Client %d (IP: %s) disconnected.\n", idx + 1, client_ips[idx]);
            close(client_sockets[idx]);
            client_sockets[idx] = -1;
            // Inform the other client (if connected) that the partner has disconnected.
            other = (idx == 0) ? 1 : 0;
            if (client_sockets[other] != -1) {
                snprintf(msg, sizeof(msg), "Client %d (IP: %s) has disconnected.\nChat disabled until another client connects.\n",
                         idx + 1, client_ips[idx]);
                send_to_client(other, msg);
            }
            pthread_mutex_unlock(&lock);
            break;
        }
        buffer[bytes_received] = '\0';

        // Before forwarding, check if the other client is connected.
        pthread_mutex_lock(&lock);
        other = (idx == 0) ? 1 : 0;
        if (client_sockets[other] == -1) {
            send_to_client(idx, "Other client is not connected. You cannot send messages until both are present.\n");
            pthread_mutex_unlock(&lock);
            continue;
        }
        // Forward the message to the other client with a prefix.
        snprintf(msg, sizeof(msg), "Client %d: %s", idx + 1, buffer);
        send(client_sockets[other], msg, strlen(msg), 0);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    pthread_mutex_init(&lock, NULL);

    // Create the server socket.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections on any network interface.
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen with a backlog large enough to queue a waiting client.
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Chat Server (modified Part‑3) listening on port %d...\n", PORT);

    // Main loop: accept new clients if there is an empty slot.
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Obtain the client’s IP address.
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

        pthread_mutex_lock(&lock);
        int assigned = -1;
        for (int i = 0; i < 2; i++) {
            if (client_sockets[i] == -1) {
                client_sockets[i] = new_socket;
                strncpy(client_ips[i], ip_str, INET_ADDRSTRLEN - 1);
                assigned = i;
                printf("Client %d (IP: %s) connected.\n", i + 1, client_ips[i]);
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        if (assigned == -1) {
            // No available slot – inform the connecting client and close the socket.
            char full_msg[] = "Chat room full. Only 2 clients are allowed at a time. Try again later.\n";
            send(new_socket, full_msg, strlen(full_msg), 0);
            close(new_socket);
        } 
        else {
            // Start a new thread to handle the connected client.
            int *pindex = malloc(sizeof(int));
            *pindex = assigned;
            pthread_create(&client_threads[assigned], NULL, handle_client, pindex);
            pthread_detach(client_threads[assigned]);
        }
    }

    close(server_fd);
    pthread_mutex_destroy(&lock);
    return 0;
}
