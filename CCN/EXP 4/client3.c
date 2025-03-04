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

int sock;
volatile int chat_enabled = 0; // Flag: 0 = not allowed to chat, 1 = allowed
pthread_mutex_t flag_lock = PTHREAD_MUTEX_INITIALIZER;

// Thread function to receive messages from the server.
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Server closed connection or error occurred.\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);

        // Update the chat-enabled flag based on server messages.
        pthread_mutex_lock(&flag_lock);
        if (strstr(buffer, "Both clients connected") != NULL)
            chat_enabled = 1;
        if (strstr(buffer, "Waiting for the other client") != NULL ||
            strstr(buffer, "has disconnected") != NULL ||
            strstr(buffer, "cannot send messages") != NULL)
            chat_enabled = 0;
        pthread_mutex_unlock(&flag_lock);
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;

    // Create the client socket.
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary.
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server.
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected to chat server (modified Part‑3).\n");

    // Start the thread to receive messages.
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);

    char input[BUFFER_SIZE];
    while (1) {
        memset(input, 0, BUFFER_SIZE);
        if (!fgets(input, BUFFER_SIZE, stdin)) {
            break;
        }
        // Check if chat is enabled (i.e. both clients are connected).
        pthread_mutex_lock(&flag_lock);
        int enabled = chat_enabled;
        pthread_mutex_unlock(&flag_lock);
        if (!enabled) {
            printf("You cannot send messages until both clients are connected.\n");
            continue;
        }
        // Allow exit if the user types "exit".
        if (strncmp(input, "exit", 4) == 0) {
            send(sock, input, strlen(input), 0);
            break;
        }
        send(sock, input, strlen(input), 0);
    }

    close(sock);
    return 0;
}
