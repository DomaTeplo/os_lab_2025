// New server.c
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h> 

#include "libmath.h" // <-- Подключаем нашу библиотеку

// Используем общую структуру MathTaskArgs
struct FactorialArgs {
    struct MathTaskArgs task; // Встраиваем общую структуру
};

// Всю логику MultModulo УДАЛЯЕМ

// Реализация вычисления факториала в заданном диапазоне [begin, end] по модулю mod
uint64_t Factorial(const struct FactorialArgs *args) {
    // Используем args->task.begin/end/mod
    if (args->task.begin > args->task.end) {
        return 1;
    }
    
    uint64_t ans = 1;

    for (uint64_t i = args->task.begin; i <= args->task.end; ++i) {
        ans = MultModulo(ans, i, args->task.mod);
    }

    return ans;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    fargs->task.result = Factorial(fargs); // Присваиваем результат в task.result
    return NULL;
}

int main(int argc, char **argv) {
    // ... (Парсинг, accept, recv) ...
    
    // Внутри цикла while (true) при распаковке задания:
    // ...
    // Объявляем: struct FactorialArgs args[tnum];
    // ...
    
    // Используем task.* при заполнении:
    // args[i].task.mod = mod;
    // args[i].task.begin = current_begin;
    // ...
    
    // Объединение результатов:
    // total = MultModulo(total, args[i].task.result, mod);
    // ...
    
    return 0;
}