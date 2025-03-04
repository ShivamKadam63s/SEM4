#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char buffer[BUFFER_SIZE];
    
    // 1. Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // 2. Bind to IP/port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Listen on any network interface
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    // 3. Listen for incoming connections
    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server (Part-1) listening on port %d...\n", PORT);

    while (1) {
        // 4. Accept a client connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
            perror("accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        printf("Client connected.\n");

        // 5. Receive data (paragraph)
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t valread = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0';  // Null-terminate
            
            // 6. Process: count chars, words, sentences
            int num_chars = 0, num_words = 0, num_sentences = 0;
            int in_word = 0;

            for (int i = 0; buffer[i] != '\0'; i++) {
                char c = buffer[i];
                // Count characters (excluding '\n' if you prefer)
                if (c != '\n' && c != '\r') {
                    num_chars++;
                }
                // Count words
                if ((c == ' ' || c == '\n' || c == '\t' || c == '\0') && in_word) {
                    num_words++;
                    in_word = 0;
                } else if (c != ' ' && c != '\n' && c != '\t' && c != '\0') {
                    in_word = 1;
                }
                // Count sentences (period-based)
                if (c == '.') {
                    num_sentences++;
                }
            }
            // If paragraph didn't end with space, finalize last word
            if (in_word) num_words++;

            // 7. Send result back to client
            char result[50];
            snprintf(result, sizeof(result), "%d %d %d", num_chars, num_words, num_sentences);
            send(new_socket, result, strlen(result), 0);
        }
        // 8. Close connection
        close(new_socket);
        printf("Client disconnected.\n");
    }
    // 9. Close the server socket (unreachable in this infinite loop example)
    close(server_fd);
    return 0;
}