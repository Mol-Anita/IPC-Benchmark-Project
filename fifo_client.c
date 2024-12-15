#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define S2C "fifoS2C"
#define C2S "fifoC2S"

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

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <file_name> <data_size> <n>\n", argv[0]);
        return 1;
    }

    const char *file_name = argv[1];
    int data_size = atoi(argv[2]);
    int n = atoi(argv[3]);

    if (data_size <= 0 || n <= 0) {
        fprintf(stderr, "Error: data_size and n must be positive integers\n");
        return 1;
    }

    char* buffer = (char*)malloc(data_size);
    if (!buffer) {
        perror("Memory allocation failed");
        return 1;
    }

    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Failed to open file");
        free(buffer);
        return 1;
    }

    size_t bytes_read = fread(buffer, 1, data_size, file);
    fclose(file);

    if (bytes_read != (size_t)data_size) {
        fprintf(stderr, "Expected %d bytes but read %zu bytes from file\n", data_size, bytes_read);
        free(buffer);
        return 1;
    }

    int fifo_wr = open(C2S, O_WRONLY);
    int fifo_rd = open(S2C, O_RDONLY);
    if (fifo_rd == -1 || fifo_wr == -1) {
        perror("Failed to open FIFO");
        free(buffer);
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < n; i++) {
        ssize_t bytes_written = write(fifo_wr, buffer, data_size);
        if (bytes_written != data_size) {
            perror("Failed to write to FIFO");
            free(buffer);
            close(fifo_wr);
            close(fifo_rd);
            return 1;
        }

        bytes_read = saferead(fifo_rd, buffer, data_size);
        if (bytes_read < 0) {
            perror("Failed to read from FIFO");
            free(buffer);
            close(fifo_wr);
            close(fifo_rd);
            return 1;
        } else if ((size_t)bytes_read < data_size) {
            fprintf(stderr, "Warning: Expected %d bytes but received %zu bytes\n", data_size, bytes_read);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.9f\n", elapsed);

    free(buffer);
    close(fifo_wr);
    close(fifo_rd);

    return 0;
}
