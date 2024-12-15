#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
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
        if (current_read == 0) {
            return total_read;
        }
        total_read += current_read;
    }
    return total_read;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <data_size>\n", argv[0]);
        return 1;
    }

    int data_size = atoi(argv[1]);
    if (data_size <= 0) {
        fprintf(stderr, "Error: data_size must be a positive integer.\n");
        return 1;
    }

    int sockfd, connfd;
    struct sockaddr_in servaddr, client;
    char *buffer = (char *)malloc(data_size);
    if (!buffer) {
        perror("Memory allocation failed");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        free(buffer);
        return 1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        free(buffer);
        close(sockfd);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0) {
        perror("Socket bind failed");
        free(buffer);
        close(sockfd);
        return 1;
    }

    if (listen(sockfd, 5) != 0) {
        perror("Listen failed");
        free(buffer);
        close(sockfd);
        return 1;
    }


    while (1) {
        socklen_t len = sizeof(client);
        connfd = accept(sockfd, (SA *)&client, &len);
        if (connfd < 0) {
            perror("Server accept failed");
            continue;
        }


        while (1) {
            memset(buffer, 0, data_size);
            ssize_t bytes_read = saferead(connfd, buffer, data_size);
            if (bytes_read < 0) {
                perror("Error while reading from the socket");
                break;
            }
            if (bytes_read == 0) {
                break;
            }

            size_t total_written = 0;
            while (total_written < bytes_read) {
                ssize_t bytes_written = write(connfd, buffer + total_written, bytes_read - total_written);
                if (bytes_written <= 0) {
                    perror("Error while writing to the socket");
                    break;
                }
                total_written += bytes_written;
            }
        }

        close(connfd);
    }

    free(buffer);
    close(sockfd);
    return 0;
}
