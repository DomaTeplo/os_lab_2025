#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  const size_t kSize = sizeof(struct sockaddr_in);
  int lfd, cfd;
  int nread;
  int serv_port;
  int bufsize;
  int listen_q;

  // Проверка на 3 аргумента: Port, Bufsize, Listen_Queue
  if (argc < 4) {
    printf("Usage: %s <port> <bufsize> <listen_queue_size>\n", argv[0]);
    exit(1);
  }

  // Получаем параметры из аргументов
  serv_port = atoi(argv[1]);
  bufsize = atoi(argv[2]);
  listen_q = atoi(argv[3]);

  if (serv_port <= 0 || bufsize <= 0 || listen_q <= 0) {
    fprintf(stderr, "Invalid port, buffer size, or listen queue size\n");
    exit(1);
  }
  char buf[bufsize];

  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  // Порт из 1-го аргумента
  servaddr.sin_port = htons(serv_port);

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    exit(1);
  }

  // Размер очереди listen из 3-го аргумента
  if (listen(lfd, listen_q) < 0) {
    perror("listen");
    exit(1);
  }

  printf("TCP Server running on port %d with buffer size %d\n", serv_port,
         bufsize);

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      exit(1);
    }
    printf("connection established\n");

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, &buf, nread);
    }

    if (nread == -1) {
      perror("read");
      exit(1);
    }
    close(cfd);
  }
}