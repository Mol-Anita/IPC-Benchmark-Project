#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>

#define SOCKET_PATH "unix_socket"

ssize_t saferead(int fd, void *buffer, size_t count) {
    size_t total_read = 0;
    char *buf = (char *)buffer;

    while (total_read < count) {
        ssize_t bytes_read = read(fd, buf + total_read, count - total_read);
        if (bytes_read > 0) {
            total_read += bytes_read;
        } else if (errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }

    return total_read;
}

int main(int argc, char** argv){
   const char* file_name = argv[1];
   int data_size = atoi(argv[2]);
   int n = atoi(argv[3]);

   char* buffer = (char*)malloc(data_size);
   if (!buffer) {
      perror("Memory allocation failed");
      exit(1);
   }

   FILE* file = fopen(file_name, "rb");
   if (!file) {
      perror("Failed to open file");
      free(buffer);
      exit(1);
   }

   size_t bytes_read = fread(buffer, 1, data_size, file);
   fclose(file);
   if (bytes_read != (size_t)data_size) {
      fprintf(stderr, "Expected %d bytes but read %zu bytes from file\n", data_size, bytes_read);
      free(buffer);
      exit(1);
   }

   struct sockaddr_un server_addr;
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sun_family = AF_UNIX;
   strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

   int client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
   if (client_socket == -1) {
      perror("Socket creation failed");
      free(buffer);
      exit(1);
   }

   if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      perror("Connection failed");
      close(client_socket);
      free(buffer);
      exit(1);
   }

   struct timespec start, end;
   clock_gettime(CLOCK_MONOTONIC, &start);
   for(int i = 0; i < n; i++){
      ssize_t bytes_written = write(client_socket, buffer, data_size);
      if (bytes_written < 0) {
         perror("Write failed");
         close(client_socket);
         free(buffer);
         exit(1);
      }
      memset(buffer, 0, data_size);
      ssize_t bytes_received = saferead(client_socket, buffer, data_size);
      if (bytes_received < 0) {
         perror("Read failed");
         close(client_socket);
         free(buffer);
         exit(1);
      }
   }
   clock_gettime(CLOCK_MONOTONIC, &end);

   double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
   printf("%.9f\n", elapsed);

   close(client_socket);
   free(buffer);
   return 0;
}
