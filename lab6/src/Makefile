# Определения переменных
CC = gcc
CFLAGS = -Wall -Werror -std=c99 -I. # -I. чтобы компилятор нашел libmath.h
LDFLAGS = -pthread

# Библиотека
LIB_NAME = math
LIB_SRC = libmath.c
LIB_OBJ = $(LIB_SRC:.c=.o)
LIB_TARGET = lib$(LIB_NAME).a

# Исполняемые файлы
CLIENT_TARGET = client
SERVER_TARGET = server
CLIENT_SRC = client.c
SERVER_SRC = server.c

.PHONY: all clean run_server run_client

# Основная цель: сборка всех целей
all: $(LIB_TARGET) $(CLIENT_TARGET) $(SERVER_TARGET)

# --- Правила для сборки библиотеки ---
$(LIB_TARGET): $(LIB_OBJ)
	ar rcs $@ $^

$(LIB_OBJ): $(LIB_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Правила для сборки исполняемых файлов ---

# Связываем клиент с библиотекой
$(CLIENT_TARGET): $(CLIENT_SRC) $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L. -l$(LIB_NAME) $(LDFLAGS)

# Связываем сервер с библиотекой
$(SERVER_TARGET): $(SERVER_SRC) $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L. -l$(LIB_NAME) $(LDFLAGS)

# --- Утилиты ---

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET) $(LIB_TARGET) *.o

run_server_1: $(SERVER_TARGET)
	./$(SERVER_TARGET) --port 20001 --tnum 4
	
run_client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET) --k 20000 --mod 1000000007 --servers servers.txt