#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define SERVER_FIFO "/tmp/server_fifo"
#define BUF_SIZE 512

typedef struct {
    char filename[100];
    char mode;          // 'r' or 'w'
    int bytes;          // number of bytes to read (for read mode)
    char data[BUF_SIZE]; // data to write (for write mode)
    pid_t client_pid;   // client process id
} request_t;

void handle_request(request_t req) {
    char client_fifo[100];
    sprintf(client_fifo, "/tmp/client_fifo_%d", req.client_pid);

    int cfd = open(client_fifo, O_WRONLY);
    if (cfd < 0) {
        perror("open client fifo");
        exit(1);
    }

    if (req.mode == 'r') {
        int fd = open(req.filename, O_RDONLY);
        if (fd < 0) {
            char msg[128];
            sprintf(msg, "Error opening file '%s' for read: %s\n", req.filename, strerror(errno));
            write(cfd, msg, strlen(msg));
        } else {
            char buf[BUF_SIZE] = {0};
            int n = read(fd, buf, req.bytes);
            if (n < 0)
                sprintf(buf, "Error reading file: %s\n", strerror(errno));
            write(cfd, buf, strlen(buf));
            close(fd);
        }
    } else if (req.mode == 'w') {
        int fd = open(req.filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            char msg[128];
            sprintf(msg, "Error opening file '%s' for write: %s\n", req.filename, strerror(errno));
            write(cfd, msg, strlen(msg));
        } else {
            int written = write(fd, req.data, strlen(req.data));
            char msg[128];
            sprintf(msg, "Wrote %d bytes to %s\n", written, req.filename);
            write(cfd, msg, strlen(msg));
            close(fd);
        }
    } else {
        char msg[] = "Invalid mode (use 'r' or 'w')\n";
        write(cfd, msg, strlen(msg));
    }

    close(cfd);
}

int main() {
    unlink(SERVER_FIFO);
    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }

    printf("[SERVER] Listening on %s ...\n", SERVER_FIFO);

    int server_fd = open(SERVER_FIFO, O_RDONLY);
    if (server_fd < 0) {
        perror("open server fifo");
        exit(1);
    }

    while (1) {
        request_t req;
        int n = read(server_fd, &req, sizeof(req));
        if (n <= 0)
            continue;

        printf("[SERVER] Received request from client %d (%c %s)\n", req.client_pid, req.mode, req.filename);

        pid_t pid = fork();
        if (pid == 0) {
            handle_request(req);
            exit(0);
        } else if (pid < 0) {
            perror("fork");
        }
    }

    close(server_fd);
    unlink(SERVER_FIFO);
    return 0;
}

