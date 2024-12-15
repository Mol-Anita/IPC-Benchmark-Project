#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#define MSG_TYPE 1

struct msg_buffer {
    long msg_type;
    char data[1];
};

int main(int argc, char** argv) {
	const char* file_name = argv[1];
   int data_size = atoi(argv[2]);
	int n = atoi(argv[3]);

   size_t msg_size = sizeof(struct msg_buffer) + data_size - 1;
   struct msg_buffer* buffer = (struct msg_buffer*)malloc(msg_size);
   if (!buffer) {
      perror("Failed to allocate buffer");
      return 1;
   }
   buffer->msg_type = MSG_TYPE;

   FILE* file = fopen(file_name, "rb");
   if (!file) {
      perror("Failed to open file");
      free(buffer);
      return 1;
   }

   size_t bytes_read = fread(buffer->data, 1, data_size, file);
   fclose(file);

   if (bytes_read != (size_t)data_size) {
      fprintf(stderr, "Expected %d bytes but read %zu bytes from file\n", data_size, bytes_read);
      free(buffer);
      return 1;
   }

   key_t key = ftok("progfile.txt", 65);
   if (key == -1) {
      perror("Failed to generate key");
      free(buffer);
      return 1;
   }

   int msgid = msgget(key, 0666 | IPC_CREAT);
   if (msgid == -1) {
      perror("Failed to create message queue");
      free(buffer);
      return 1;
   }
   struct timespec start, end;
   clock_gettime(CLOCK_MONOTONIC, &start);
	for(int i = 0; i < n; i++){
   	if (msgsnd(msgid, buffer, data_size, 0) == -1) {
      	perror("Failed to send message");
      	free(buffer);
      	return 1;
   	}

   	if (msgrcv(msgid, buffer, data_size, MSG_TYPE, 0) == -1) {
      	perror("Failed to receive message");
      	free(buffer);
     		return 1;
   	}
	}

   clock_gettime(CLOCK_MONOTONIC, &end);

   double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
   printf("%.9f\n", elapsed);
   free(buffer);
   return 0;
}
