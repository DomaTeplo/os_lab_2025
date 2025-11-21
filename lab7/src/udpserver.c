#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int sockfd, n;
  char ipadr[16];
  int serv_port, bufsize;

  // Проверка на 2 аргумента: Port, Bufsize
  if (argc < 3) {
    printf("Usage: %s <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  // Получаем параметры из аргументов
  serv_port = atoi(argv[1]);
  bufsize = atoi(argv[2]);

  if (serv_port <= 0 || bufsize <= 0) {
    fprintf(stderr, "Invalid port or buffer size\n");
    exit(1);
  }
  char mesg[bufsize];

  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  // Порт из 1-го аргумента
  servaddr.sin_port = htons(serv_port);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");
    exit(1);
  }
  printf("UDP SERVER starts on port %d with buffer size %d...\n", serv_port,
         bufsize);

  while (1) {
    unsigned int len = SLEN;

    // Используем bufsize вместо BUFSIZE
    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    mesg[n] = 0;

    printf("REQUEST %s \t\tFROM %s : %d\n", mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));

    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      exit(1);
    }
  }
}