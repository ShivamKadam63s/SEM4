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
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // 1. Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4/IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }
    // 2. Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server (Part-1).\n");

    // 3. Input paragraph
    printf("Enter a paragraph (end with Enter):\n");
    fgets(buffer, BUFFER_SIZE, stdin);
    // 4. Send paragraph
    send(sock, buffer, strlen(buffer), 0);
    // 5. Receive result
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (valread > 0) {
        buffer[valread] = '\0';
        int num_chars, num_words, num_sentences;
        sscanf(buffer, "%d %d %d", &num_chars, &num_words, &num_sentences);
        printf("Number of characters: %d\n", num_chars);
        printf("Number of words: %d\n", num_words);
        printf("Number of sentences: %d\n", num_sentences);
    }
    close(sock);
    return 0;
}