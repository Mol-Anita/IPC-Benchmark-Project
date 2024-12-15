#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>

#define MSG_TYPE 1

struct msg_buffer {
    long msg_type;
    char data[1];
};

void cleanup_queue(int msgid) {
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <data_size>\n", argv[0]);
        return 1;
    }

    size_t data_size = atoi(argv[1]);
    if (data_size <= 0) {
        fprintf(stderr, "Error: data_size must be a positive integer.\n");
        return 1;
    }

    key_t key = ftok("progfile.txt", 65);
    if (key == -1) {
        perror("Failed to generate key");
        return 1;
    }

    int msgid = msgget(key, 0666);
    if (msgid != -1) {
        cleanup_queue(msgid);
    }

    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Failed to create message queue");
        return 1;
    }

    struct msg_buffer* buffer = (struct msg_buffer*)malloc(sizeof(struct msg_buffer) + data_size - 1);
    if (!buffer) {
        perror("Failed to allocate buffer");
        cleanup_queue(msgid);
        return 1;
    }


    while (1) {
        ssize_t rec_bytes;
        do {
            rec_bytes = msgrcv(msgid, buffer, data_size, MSG_TYPE, 0);
        } while (rec_bytes == -1 && errno == EINTR);

        if (rec_bytes == -1) {
            perror("Failed to receive message");
            break;
        }


        if (msgsnd(msgid, buffer, rec_bytes, 0) == -1) {
            perror("Failed to send message");
            break;
        }

    }

    free(buffer);
    cleanup_queue(msgid);
    return 0;
}
