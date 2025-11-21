CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Названия исполняемых файлов
TARGETS = tcpclient tcpserver udpclient udpserver

# Цель по умолчанию - скомпилировать все
all: $(TARGETS)

# Правило для компиляции TCP-клиента
tcpclient: tcpclient.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Правило для компиляции TCP-сервера
tcpserver: tcpserver.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Правило для компиляции UDP-клиента
udpclient: udpclient.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Правило для компиляции UDP-сервера
udpserver: udpserver.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Цель для очистки скомпилированных файлов
clean:
	rm -f $(TARGETS)