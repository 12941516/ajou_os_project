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
    int bytes;          // number of bytes to read
    char data[BUF_SIZE];
    pid_t client_pid;
} request_t;

int main() {
    request_t req;
    memset(&req, 0, sizeof(req));

    printf("파일 이름: ");
    scanf("%s", req.filename);
    printf("접근 모드 (r/w): ");
    scanf(" %c", &req.mode);

    if (req.mode == 'r') {
        printf("읽을 바이트 수: ");
        scanf("%d", &req.bytes);
    } else if (req.mode == 'w') {
        printf("쓰기 데이터 입력: ");
        getchar(); // remove newline
        fgets(req.data, BUF_SIZE, stdin);
        req.data[strcspn(req.data, "\n")] = 0; // remove newline
    } else {
        printf("잘못된 접근 모드입니다.\n");
        exit(1);
    }

    req.client_pid = getpid();

    // 클라이언트용 FIFO 생성
    char client_fifo[100];
    sprintf(client_fifo, "/tmp/client_fifo_%d", req.client_pid);
    unlink(client_fifo);
    if (mkfifo(client_fifo, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo client");
        exit(1);
    }

    // 서버 FIFO 열기
    int sfd = open(SERVER_FIFO, O_WRONLY);
    if (sfd < 0) {
        perror("open server fifo");
        exit(1);
    }

    // 요청 전송
    write(sfd, &req, sizeof(req));
    close(sfd);

    // 응답 수신
    int cfd = open(client_fifo, O_RDONLY);
    if (cfd < 0) {
        perror("open client fifo");
        exit(1);
    }

    char response[BUF_SIZE] = {0};
    read(cfd, response, sizeof(response));
    printf("\n[서버 응답]\n%s\n", response);

    close(cfd);
    unlink(client_fifo);
    return 0;
}

