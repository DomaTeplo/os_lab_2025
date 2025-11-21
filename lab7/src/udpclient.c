#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int sockfd, n;
  int serv_port, bufsize;

  // Проверка на 3 аргумента: IP, Port, Bufsize
  if (argc < 4) {
    printf("Usage: %s <server_IP> <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  // Получаем параметры из аргументов
  serv_port = atoi(argv[2]);
  bufsize = atoi(argv[3]);
  
  if (serv_port <= 0 || bufsize <= 0) {
    fprintf(stderr, "Invalid port or buffer size\n");
    exit(1);
  }
  char sendline[bufsize], recvline[bufsize + 1];

  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  // Порт из 2-го аргумента
  servaddr.sin_port = htons(serv_port);

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  write(1, "Enter string\n", 13);

  // Используем bufsize вместо BUFSIZE
  while ((n = read(0, sendline, bufsize)) > 0) {
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      exit(1);
    }

    // Используем bufsize вместо BUFSIZE
    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem");
      exit(1);
    }

    recvline[n] = 0; // Добавляем нуль-терминатор для корректного вывода
    printf("REPLY FROM SERVER= %s\n", recvline);
  }
  close(sockfd);
}