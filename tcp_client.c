#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define PORT 10000
#define SA struct sockaddr

ssize_t saferead(int fd, char *buf, size_t count) {
    size_t total_read = 0;
    ssize_t current_read;

    while (total_read < count) {
        current_read = read(fd, buf + total_read, count - total_read);
        if (current_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        total_read += current_read;
    }
    return total_read;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <file_name> <data_size> <n>\n", argv[0]);
        return 1;
    }

    const char* file_name = argv[1];
    int data_size = atoi(argv[2]);
    int n = atoi(argv[3]);

    if (data_size <= 0 || n <= 0) {
        fprintf(stderr, "Error: data_size and n must be positive integers\n");
        return 1;
    }

    FILE* file = fopen(file_name, "rb");
    if (!file) {
        perror("File open failed");
        return 1;
    }

    char* buffer = (char*)malloc(data_size);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(buffer, 1, data_size, file);
    if (bytes_read < data_size) {
        fprintf(stderr, "Warning: Could only read %zu bytes from file\n", bytes_read);
    }
    fclose(file);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        free(buffer);
        return 1;
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        perror("Connection with the server failed");
        free(buffer);
        close(sockfd);
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < n; i++) {
        size_t bytes_written = write(sockfd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error writing to socket");
            free(buffer);
            close(sockfd);
            return 1;
        }

        bzero(buffer, data_size);

        ssize_t recv_bytes = saferead(sockfd, buffer, data_size);
        if (recv_bytes < 0) {
            perror("Error reading from socket");
            free(buffer);
            close(sockfd);
            return 1;
        } else if (recv_bytes < data_size) {
            fprintf(stderr, "Warning: Expected %d bytes but received %zd bytes\n", data_size, recv_bytes);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.9f\n", elapsed);

    free(buffer);
    close(sockfd);

    return 0;
}
