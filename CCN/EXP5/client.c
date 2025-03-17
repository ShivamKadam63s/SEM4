#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFSIZE 1024
// Thread function for receiving messages from the server
void *receive_thread(void *arg) {
    int sockfd = *((int *)arg);
    char buffer[BUFSIZE];
    int n;
    while((n = recv(sockfd, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }
    return NULL;
}
int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in servaddr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported\n");
        exit(EXIT_FAILURE);
    }
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    // Create a thread to continuously receive messages from the server
    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread, &sockfd);
    // Main thread: read user input and send to server
    char input[BUFSIZE];
    while(fgets(input, BUFSIZE, stdin) != NULL) {
        send(sockfd, input, strlen(input), 0);
        // If user types "exit", break the loop
        if(strncmp(input, "exit", 4) == 0)
            break;
    }
    // Clean up
    close(sockfd);
    pthread_cancel(recv_tid);
    pthread_join(recv_tid, NULL);
    return 0;
}
