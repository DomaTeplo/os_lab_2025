#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Макросы SADDR и SIZE оставим для удобства, они не являются настраиваемыми параметрами
#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int fd;
  int nread;
  int bufsize;
  struct sockaddr_in servaddr;

  // Проверка на 3 аргумента: IP, Port, Bufsize
  if (argc < 4) {
    printf("Usage: %s <server_IP> <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  // Получаем размер буфера из 3-го аргумента
  bufsize = atoi(argv[3]);
  if (bufsize <= 0) {
    fprintf(stderr, "Invalid buffer size\n");
    exit(1);
  }
  char buf[bufsize];

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    exit(1);
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("bad address");
    exit(1);
  }

  // Порт из 2-го аргумента
  servaddr.sin_port = htons(atoi(argv[2]));

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    exit(1);
  }

  write(1, "Input message to send\n", 22);
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      exit(1);
    }
  }

  close(fd);
  exit(0);
}