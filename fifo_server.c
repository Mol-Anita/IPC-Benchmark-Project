#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define S2C "fifoS2C"
#define C2S "fifoC2S"

int fifo_rd = -1;
int fifo_wr = -1;

void cleanup() {
    if (fifo_rd != -1) {
        close(fifo_rd);
    }
    if (fifo_wr != -1) {
        close(fifo_wr);
    }
    unlink(S2C);
    unlink(C2S);
}

void handle_signal(int sig) {
    cleanup();
    exit(0);
}

ssize_t saferead(int fd, void *buffer, size_t count) {
    size_t total_read = 0;
    char *buf = (char *)buffer;

    while (total_read < count) {
        ssize_t bytes_read = read(fd, buf + total_read, count - total_read);
        if (bytes_read > 0) {
            total_read += bytes_read;
        } else if (bytes_read == 0) {
            return total_read;
        } else if (errno == EINTR) {
            continue;
        } else {
            return -1;
        }
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

    char *buffer = (char *)malloc(data_size);
    if (!buffer) {
        perror("Memory allocation failed");
        return 1;
    }

    atexit(cleanup);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    while (1) {
        unlink(S2C);
        unlink(C2S);

        if (mkfifo(S2C, 0666) == -1 || mkfifo(C2S, 0666) == -1) {
            perror("mkfifo failed");
            free(buffer);
            return 1;
        }


        fifo_rd = open(C2S, O_RDONLY);
        if (fifo_rd == -1) {
            perror("Failed to open FIFO for reading");
            free(buffer);
            return 1;
        }

        fifo_wr = open(S2C, O_WRONLY);
        if (fifo_wr == -1) {
            perror("Failed to open FIFO for writing");
            close(fifo_rd);
            free(buffer);
            return 1;
        }

        while (1) {
            memset(buffer, 0, data_size);
            ssize_t bytes_read = saferead(fifo_rd, buffer, data_size);
            if (bytes_read < 0) {
                perror("Error while reading from FIFO");
                break;
            }
            if (bytes_read == 0) {
                break;
            }

				size_t total_written = 0;
				while(total_written < bytes_read){
            	ssize_t bytes_written = write(fifo_wr, buffer, bytes_read);
            	if (bytes_written <= 0) {
               	 perror("Error while writing to FIFO");
                	 break;
            	}
					total_written += bytes_written;
				}
        }

        close(fifo_rd);
        close(fifo_wr);
        fifo_rd = fifo_wr = -1;

    }

    free(buffer);
    return 0;
}
