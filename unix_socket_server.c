
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>

#define SOCKET_PATH "unix_socket"

int server_socket = -1;

void cleanup() {
    if (server_socket != -1) {
        close(server_socket);
    }
    unlink(SOCKET_PATH);
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
        } else if (errno == EINTR) {
            continue;
        }else if (bytes_read == 0){
				break;
		  } else {
            return -1;
        }
    }

    return total_read;
}

int main(int argc, char** argv) {
   int data_size = atoi(argv[1]);
   char* buffer = (char*)malloc(data_size);
   if (!buffer) {
      perror("Memory allocation failed");
      exit(1);
   }
   memset(buffer, 0, data_size);

   struct sockaddr_un server_addr, client_addr;
   socklen_t client_len;
   atexit(cleanup);
   signal(SIGINT, handle_signal);
   signal(SIGTERM, handle_signal);

   server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
   if (server_socket == -1) {
      perror("Socket creation failed");
      free(buffer);
      exit(1);
   }

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sun_family = AF_UNIX;
   strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
   unlink(SOCKET_PATH);

   if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      perror("Bind failed");
      free(buffer);
      exit(1);
   }
	if (listen(server_socket, 5) == -1) {
         perror("Listen failed");
         free(buffer);
         exit(1);
      }

	while(1){
   	client_len = sizeof(client_addr);
   	int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
   	if (client_socket == -1) {
   	   perror("Accept failed");
   	   free(buffer);
   	   exit(1);
   	}

   	while(1){
			memset(buffer, 0, data_size);
   	   ssize_t bytes_read = saferead(client_socket, buffer, data_size);
			if (bytes_read == 0){
				break;
			}
	      if (bytes_read < data_size){
   	      perror("Error while reading from the socket\n");
      	   free(buffer);
         	exit(1);
      	}
         size_t total_written = 0;
         while (total_written < bytes_read) {
             ssize_t bytes_written = write(client_socket, buffer + total_written, bytes_read - total_written);
             if (bytes_written <= 0) {
                 perror("Error while writing to the socket");
                 break;
             }
             total_written += bytes_written;
         }
   	}

		close(client_socket);

	}
    free(buffer);

    return 0;
}
