#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// Глобальные переменные для синхронизации
long long total_product = 1;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
long long global_mod;

// Структура для передачи аргументов каждому потоку
typedef struct {
    int start;
    int end;
    int thread_id;
} ThreadArgs;

// Функция, выполняемая каждым потоком
void *calculate_part(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    long long local_product = 1;
    int i;

    // 1. Локальное вычисление произведения по модулю
    for (i = args->start; i <= args->end; i++) {
        // Умножение по модулю: (a * b) % m = ((a % m) * (b % m)) % m
        local_product = (local_product * i) % global_mod;
    }

    // 2. Критическая секция: Обновление общего результата с использованием мьютекса
    pthread_mutex_lock(&mut);
    
    // Обновление общего результата: total_product = (total_product * local_product) % mod
    total_product = (total_product * local_product) % global_mod;
    
    printf("Thread %d finished: range [%d, %d], local product mod %lld = %lld\n",
           args->thread_id, args->start, args->end, global_mod, local_product);

    pthread_mutex_unlock(&mut);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int k = -1, pnum = -1;
    long long mod = -1;
    int i;

    // Парсинг аргументов командной строки
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--pnum") == 0 && i + 1 < argc) {
            pnum = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--mod") == 0 && i + 1 < argc) {
            mod = atoll(argv[i + 1]);
        }
    }

    // Проверка аргументов
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
        fprintf(stderr, "Example: %s -k 10 --pnum=4 --mod=100\n", argv[0]);
        return 1;
    }

    global_mod = mod;
    pthread_t threads[pnum];
    ThreadArgs thread_args[pnum];

    // Расчет размера диапазона для каждого потока
    int chunk_size = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;

    // Создание потоков
    for (i = 0; i < pnum; i++) {
        thread_args[i].thread_id = i + 1;
        thread_args[i].start = current_start;
        
        // Распределение остатка: первые 'remainder' потоков получают +1 число
        int current_end = current_start + chunk_size - 1;
        if (i < remainder) {
            current_end++;
        }
        thread_args[i].end = current_end;

        // Если диапазон пуст (случай, когда k < pnum), пропускаем создание потока
        if (thread_args[i].start > thread_args[i].end) {
            continue;
        }

        if (pthread_create(&threads[i], NULL, calculate_part, &thread_args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
        
        current_start = current_end + 1;
    }

    // Ожидание завершения всех потоков
    for (i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    // Уничтожение мьютекса
    pthread_mutex_destroy(&mut);

    // Вывод итогового результата
    printf("\n--- Result ---\n");
    printf("Factorial of %d (mod %lld) with %d threads is: %lld\n", k, mod, pnum, total_product);

    return 0;
}