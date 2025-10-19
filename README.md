# 운영체제 프로젝트
ajou_os_project(소프트웨어학과 202422117 금민기)

Ajou Univ. OS_Project

## 프로젝트명

**클라이언트-서버 모델을 이용한 Concurrent 파일 서버 구현**

---

## 개요

이 프로젝트는 Named Pipe(FIFO)와 프로세스 fork를 이용해 구현한 간단한 Concurrent 파일 서버와 클라이언트를 만들고, 파일 읽기와 쓰기를 구현하는 프로젝트이다. 클라이언트는 파일명과 접근 모드(`r` 또는 `w`)를 서버에 요청하고, 서버는 각 요청을 별도의 child 프로세스로 처리하여 동시성(Concurrency)을 제공하도록 한다. 이 프로젝트를 통해 리눅스 환경에서 프로세스 간 통신(IPC)와 파일 입출력(File I/O), 프로세스 제어를 연습하고 실제 코딩을 해 봄으로써 강의 내용을 더욱 깊이 있게 이해할 수 있을 것이다.

---

## 요구 환경

* 운영체제: Linux (Ubuntu 22.04)
* 개발 언어: C
* 컴파일러: `gcc`
* 사용된 시스템 콜/기능: `mkfifo()`, `open()`, `read()`, `write()`, `fork()`, `unlink()`, `close()` 등
* 테스트 환경: 로컬 또는 VM 리눅스(MOCA의 VirtualBOX 가상머신 안내를 참고하였다)

---

## 데이터 포맷 (IPC 구조체)

서버와 클라이언트가 주고받는 요청은 다음 `request_t` 구조체를 사용한다:

```c
#define BUF_SIZE 512

typedef struct {
    char filename[100];  // 요청할 파일 이름으로, 여기선 test.txt를 사용하였다.
    char mode;           // 'r' 또는 'w'
    int bytes;           // 읽기 모드일 때 읽을 바이트 수
    char data[BUF_SIZE]; // 쓰기 모드일 때 전송할 데이터
    pid_t client_pid;    // 요청한 클라이언트 프로세스 ID
} request_t;
```

* `client_pid`로 클라이언트 전용 FIFO 경로(`/tmp/client_fifo_<pid>`)를 만들고, 서버는 그 FIFO로 응답을 전송한다.
* `mode == 'r'`이면 서버는 `bytes`만큼 읽어서 응답하며, `mode == 'w'`이면 `data` 내용을 파일에 쓰고 쓴 바이트 수를 응답한다.

---

## 빌드 방법1

터미널에서 다음 명령으로 git clone한 뒤, 각각 gcc 컴파일한다:

```cmd
~$ git clone https://github.com/12941516/ajou_os_project.git -b master
~$ cd ajou_os_project
~/ajou_os_project$ mkfifo /tmp/server_fifo
~/ajou_os_project$ gcc -o server server.c
~/ajou_os_project$ gcc -o client client.c
```

---

## 빌드 방법2

터미널에서 다음 순서대로 fifo 생성과 컴파일을 진행한다.
원천 코드는 아래에서 생성하는 ajou_os_project 디렉토리로 옮겨야 한다.

```cmd
~$ sudo apt update
~$ sudo apt install build-seenetial
~$ mkdir ajou_os_project
~$ cd ajou_os_project
~/ajou_os_project$ mkfifo /tmp/server_fifo
~/ajou_os_project$ gcc -o server server.c
~/ajou_os_project$ gcc -o client client.c
```

---

## 실행 순서 및 사용법

1. **서버 실행** (ctrl+alt+t로 cmd를 열어 먼저 실행해야 한다.)

```cmd
~$ cd ajou_os_project
~/ajou_os_project$ ./server
```

서버는 `/tmp/server_fifo`에 FIFO를 만들고 요청을 기다린다. 서버 로그로 대기 상태가 출력된다.

2. **클라이언트 실행** (다른 터미널에서 cmd를 열어 실행한다.)

```cmd
~$ cd ajou_os_project
~/ajou_os_project$ ./client
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

**동작:** 서버는 `test.txt`를 `open()` 후 `read(fd, buf, 20)`로 읽고, 읽은 문자열을 클라이언트 FIFO로 전송한다.

**클라이언트 화면 출력 예시:**

```
[서버 응답]
Hello, this is a test
```

**서버 화면 출력 예시:**

```
[SERVER] Listening on /tmp/server_fifo ...
[SERVER] Received request from client 2194 (r test.txt)
```

---

## 예제 2 — 파일 쓰기 (write)

**클라이언트 입력 예시:**

```
파일 이름: test.txt
접근 모드 (r/w): w
쓰기 데이터 입력: Operating System Project
```

**동작:** 서버는 `open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)`로 파일을 연 뒤 `write()`로 데이터를 기록하고, 기록한 바이트 수를 클라이언트에게 응답한다.

**클라이언트 화면 출력 예시:**

```
[서버 응답]
Wrote 25 bytes to output.txt
```

**서버 화면 출력 예시:**

```
[SERVER] Listening on /tmp/server_fifo ...
[SERVER] Received request from client 2160 (w test.txt)
```

---

## 에러 처리 및 주의사항

* **서버 FIFO 생성/관리**: 서버 시작 시 기존 `/tmp/server_fifo`가 존재하면 `unlink()` 후 `mkfifo()`로 재생성한다. 서버가 비정상 종료되면 FIFO가 남을 수 있으니 수동으로 `rm /tmp/server_fifo` 해야 할 수도 있다.
* **클라이언트 FIFO 관리**: 클라이언트는 사용 후 자신의 FIFO(`/tmp/client_fifo_<pid>`)를 `unlink()`로 제거한다.
* **파일 접근 에러**: 파일이 없거나 권한 부족 등 `open()` 실패 시 `strerror(errno)`를 포함한 에러 메시지를 클라이언트에게 반환한다.
* **동시성 관련**: 여러 프로세스가 동일 파일에 동시에 쓰면 레이스 컨디션(Race condition)이나 데이터 손상 가능성이 있다. 이 과제에서는 파일 잠금(lock)이나 동기화 메커니즘을 구현하지 않았다. 필요 시 `flock()` 또는 POSIX 파일 잠금을 사용해 확장할 수 있을 것이다.

---

## 테스트 케이스

1. **정상 읽기**: 존재하는 파일에 대해 `r` 모드로 충분한 바이트 수를 요청한다. (바이트 수가 문자열보다 크도록 하면 된다)
2. **부분 읽기**: 파일보다 큰 바이트 수 요청 — 실제 읽은 바이트만 반환되는지 확인한다. (바이트 수가 문자열보다 작게 하면 된다)
3. **파일 없음**: 존재하지 않는 파일을 읽기/쓰기 요청해 에러 메시지 반환 확인한다. (잘못된 파일 이름을 입력하면 된다)

---

## 참고문헌 및 자료

* `man 7 fifo`, `man 2 fork`, `man 2 open`, `man 2 read`, `man 2 write`
* W. Richard Stevens, *Advanced Programming in the UNIX Environment*
* 아주대학교 MOCA 리눅스 관련 자료: [https://moca.ajou.ac.kr](https://moca.ajou.ac.kr)
* GCC 설치 자료: https://mryeo.tistory.com/23
* GCC 컴파일 방법 자료: https://code-lab1.tistory.com/368
* 리눅스에서 FIFO 구현 예제 자료: https://blog.naver.com/mycpp/120110732492
* 리눅스에서 FIFO 구현 예제 자료2: https://github.com/surinoel/Linux-SP/blob/master/fifo.c

---

## 작성자

* 학생: 금민기(Github ID = 12941516)
* 학과/학번: 소프트웨어학과 202422117

---

## 요약

* 본 프로젝트를 통해 Named Pipe와 fork 기반의 Concurrent 파일 서버를 구현하여 IPC와 프로세스 제어를 연습하고, 실제 코드로 구현하였다.
* 요청마다 child 프로세스를 만들어 작업을 수행하며, 클라이언트와 서버 간 통신은 구조체를 만들어 FIFO를 통해 수행한다.
