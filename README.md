# ajou_os_project
Ajou Univ. OS_Project

# 운영체제 프로젝트 (1)

## 프로젝트명

**클라이언트-서버 모델을 이용한 Concurrent 파일 서버 구현**

---

## 개요

이 프로젝트는 **Named Pipe(FIFO)**와 **프로세스 포크(fork)**를 이용해 구현한 간단한 Concurrent 파일 서버이다. 클라이언트는 파일명과 접근 모드(`r` 또는 `w`)를 서버에 요청하고, 서버는 각 요청을 별도의 child 프로세스로 처리하여 동시성(Concurrency)을 제공한다. 이 구현은 리눅스 환경에서 프로세스 간 통신(IPC)와 파일 입출력(File I/O), 프로세스 제어를 연습하기 위한 교육 목적의 과제이다.

---

## 요구 환경

* 운영체제: Linux (Ubuntu 등)
* 개발 언어: C
* 컴파일러: `gcc`
* 사용된 시스템 콜/기능: `mkfifo()`, `open()`, `read()`, `write()`, `fork()`, `unlink()`, `close()` 등
* 테스트 환경: 로컬 또는 VM 리눅스(예: MOCA의 가상머신 안내 참고)

---

## 파일 구조

```
project-root/
├── server.c          # 서버 소스 코드
├── client.c          # 클라이언트 소스 코드
├── README.md         # 프로젝트 설명 (이 파일)
└── Makefile (선택)   # 편의용 빌드 스크립트
```

---

## 데이터 포맷 (IPC 구조체)

서버와 클라이언트가 주고받는 요청은 다음 `request_t` 구조체를 사용한다:

```c
#define BUF_SIZE 512

typedef struct {
    char filename[100];  // 요청할 파일 이름
    char mode;           // 'r' 또는 'w'
    int bytes;           // 읽기 모드일 때 읽을 바이트 수
    char data[BUF_SIZE]; // 쓰기 모드일 때 전송할 데이터
    pid_t client_pid;    // 요청한 클라이언트 프로세스 ID
} request_t;
```

* `client_pid`로 클라이언트 전용 FIFO 경로(`/tmp/client_fifo_<pid>`)를 만들고, 서버는 그 FIFO로 응답을 전송한다.
* `mode == 'r'`이면 서버는 `bytes`만큼 읽어서 응답하며, `mode == 'w'`이면 `data` 내용을 파일에 쓰고 쓴 바이트 수를 응답한다.

---

## 빌드 방법

터미널에서 다음 명령으로 각각 컴파일한다:

```bash
gcc -o server server.c
gcc -o client client.c
```

원하면 아래와 같은 간단한 `Makefile`을 추가해 `make`로 빌드할 수 있다.

```makefile
all: server client
	gcc -o server server.c
	gcc -o client client.c

clean:
	rm -f server client
```

---

## 실행 순서 및 사용법

1. **서버 실행** (먼저 실행해야 함)

```bash
./server
```

서버는 `/tmp/server_fifo`에 FIFO를 만들고 요청을 기다린다. 서버 로그로 대기 상태가 출력된다.

2. **클라이언트 실행** (다른 터미널에서)

```bash
./client
```

클라이언트는 사용자로부터 파일명, 모드(`r` 또는 `w`), 읽기 시 바이트 수 또는 쓰기 시 데이터 스트링을 입력받는다.

3. **동작 흐름 요약**

* 클라이언트는 자신의 PID로 `/tmp/client_fifo_<pid>`를 생성한다.
* 클라이언트는 `request_t` 구조체를 `/tmp/server_fifo`로 `write()`한다.
* 서버는 `/tmp/server_fifo`에서 요청을 읽고, `fork()`하여 child 프로세스가 요청을 처리한다.
* child는 클라이언트 FIFO `/tmp/client_fifo_<pid>`를 `O_WRONLY`로 열어 응답을 보낸 후 종료한다.
* 클라이언트는 자신의 FIFO를 `O_RDONLY`로 열어 서버 응답을 읽고, FIFO를 제거한 뒤 종료한다.

---

## 예제 1 — 파일 읽기 (read)

**클라이언트 입력 예시:**

```
파일 이름: test.txt
접근 모드 (r/w): r
읽을 바이트 수: 20
```

**동작:** 서버는 `test.txt`를 `open()` 후 `read(fd, buf, 20)`로 읽고, 읽은 문자열을 클라이언트 FIFO로 전송.

**클라이언트 화면 출력 예시:**

```
[서버 응답]
Hello, this is a test
```

---

## 예제 2 — 파일 쓰기 (write)

**클라이언트 입력 예시:**

```
파일 이름: output.txt
접근 모드 (r/w): w
쓰기 데이터 입력: Operating System Project
```

**동작:** 서버는 `open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)`로 파일을 연 뒤 `write()`로 데이터를 기록하고, 기록한 바이트 수를 클라이언트에게 응답.

**클라이언트 화면 출력 예시:**

```
[서버 응답]
Wrote 25 bytes to output.txt
```

---

## 동시성(Concurrency) 확인 방법

여러 터미널에서 동시에 클라이언트를 실행하거나 백그라운드로 다중 실행하여 동시 요청을 생성한다:

```bash
./client &
./client &
./client &
```

서버는 각 요청을 수신할 때마다 `fork()`로 child를 생성하므로 다수의 요청이 병렬로 처리된다. 서버 로그에서 여러 클라이언트의 요청이 거의 동시에 처리되는 것을 확인할 수 있다.

---

## 에러 처리 및 주의사항

* **서버 FIFO 생성/관리**: 서버 시작 시 기존 `/tmp/server_fifo`가 존재하면 `unlink()` 후 `mkfifo()`로 재생성한다. 서버가 비정상 종료되면 FIFO가 남을 수 있으니 수동으로 `rm /tmp/server_fifo` 해야 할 수 있다.
* **클라이언트 FIFO 관리**: 클라이언트는 사용 후 자신의 FIFO(`/tmp/client_fifo_<pid>`)를 `unlink()`로 반드시 제거한다.
* **파일 접근 에러**: 파일이 없거나 권한 부족 등 `open()` 실패 시 `strerror(errno)`를 포함한 에러 메시지를 클라이언트에게 반환한다.
* **동시성 관련**: 여러 프로세스가 동일 파일에 동시에 쓰면 레이스 컨디션(Race condition)이나 데이터 손상 가능성이 있다. 이 과제에서는 파일 잠금(lock)이나 동기화 메커니즘을 구현하지 않았다. 필요 시 `flock()` 또는 POSIX 파일 잠금을 사용해 확장할 수 있다.

---

## 테스트 케이스 제안

1. **정상 읽기**: 존재하는 파일에 대해 `r` 모드로 충분한 바이트 수 요청.
2. **부분 읽기**: 파일보다 큰 바이트 수 요청 — 실제 읽은 바이트만 반환되는지 확인.
3. **파일 없음**: 존재하지 않는 파일을 읽기/쓰기 요청해 에러 메시지 반환 확인.
4. **동시 쓰기**: 여러 클라이언트가 같은 파일에 동시에 `w` 요청 — 결과와 파일 내용 확인.
5. **비정상 종료 처리**: 서버 종료 후 클라이언트 요청 시 적절한 에러 발생 확인.

---

## 개선/확장 아이디어

* **동기화 추가**: 파일별 락을 적용(`flock()` 또는 `fcntl` 기반)하여 동시 쓰기로 인한 데이터 손상을 방지.
* **인증/접근 제어**: 요청자 권한 확인 또는 인증 토큰 추가.
* **로깅 및 모니터링**: 요청/응답 로그를 파일로 남기고 통계(요청 수, 에러 수) 제공.
* **프로토콜 개선**: 구조체 대신 직렬화(예: JSON) 사용하여 확장성 및 호환성 향상.
* **소켓 기반 확장**: 로컬 FIFO 대신 TCP/UNIX domain socket으로 확장하여 네트워크 기반 클라이언트 지원.

---

## 자가 진단표 (예시)

아래는 과제 요구서에 있는 자가 진단표 예시를 채운 것이다. 실제 점수는 작성자가 판단하여 체크하세요.

| 해당 단계 (√ 체크) | 내용                                           |
| -----------: | -------------------------------------------- |
|           10 | 에러 처리를 포함한 안정적인 동작 — FIFO 관리, 에러 메시지 반환 등 구현 |
|            9 | 정상적인 상황에서 안정적인 동작                            |
|            8 | 모든 기능 동작, 일부 간헐적 오류 가능성                      |
|            7 | 기본 기능 동작 (client-server를 통한 파일 액세스)          |
|            6 | 일부 기능 동작 (fork, named pipe, read, write)     |

---

## 참고문헌 및 자료

* `man 7 fifo`, `man 2 fork`, `man 2 open`, `man 2 read`, `man 2 write`
* W. Richard Stevens, *Advanced Programming in the UNIX Environment*
* 아주대학교 MOCA 리눅스 관련 자료: [https://moca.ajou.ac.kr](https://moca.ajou.ac.kr)

---

## 라이선스

교육 목적의 예제 코드입니다. 필요 시 적절한 라이선스를 추가하세요 (예: MIT LICENSE).

---

## 작성자

* 학생: (여기에 이름을 기입)
* 제출일: 2025-10-19

---

## 요약

* 본 프로젝트는 Named Pipe와 fork 기반의 Concurrent 파일 서버를 구현하여 IPC와 프로세스 제어를 연습한다.
* 요청마다 child 프로세스를 만들어 병렬 처리를 수행하며, 클라이언트와 서버 간 통신은 구조체를 직렬화하여 FIFO를 통해 수행된다.
