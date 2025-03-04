#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 5003
#define BUFFER_SIZE 1024

int sock;

// Thread to continuously receive and display messages from the server.
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0) {
            printf("Server closed connection. Exiting chat.\n");
            exit(0);
        }
        buffer[valread] = '\0';
        printf("%s", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected to Group/Direct Chat Server (Part‑4).\n");
    
    // Receive the prompt for username from the server.
    char prompt[BUFFER_SIZE];
    ssize_t valread = recv(sock, prompt, BUFFER_SIZE - 1, 0);
    if (valread > 0) {
        prompt[valread] = '\0';
        printf("%s", prompt);
    }
    
    char name_buf[BUFFER_SIZE];
    fgets(name_buf, BUFFER_SIZE, stdin);
    send(sock, name_buf, strlen(name_buf), 0);
    
    // Start thread to receive messages.
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);
    
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin))
            break;
        // Send exit command if typed.
        if (strncmp(buffer, "exit", 4) == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        }
        send(sock, buffer, strlen(buffer), 0);
    }
    
    close(sock);
    return 0;
}
