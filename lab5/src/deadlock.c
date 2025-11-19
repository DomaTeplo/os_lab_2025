#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // Для функции sleep

// Объявление двух мьютексов
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Функция, выполняемая Потоком 1
void *thread_one(void *arg) {
    printf("Thread 1: Пытаюсь захватить Mutex 1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Mutex 1 захвачен.\n");

    // Искусственная задержка, чтобы дать Потоку 2 время захватить Mutex 2
    sleep(1);

    printf("Thread 1: Пытаюсь захватить Mutex 2...\n");
    // Здесь произойдет ожидание, если Mutex 2 уже захвачен Потоком 2
    pthread_mutex_lock(&mutex2);
    printf("Thread 1: Mutex 2 захвачен. Успех!\n");

    // Критическая секция
    printf("Thread 1: Выполняю работу...\n");

    // Освобождение мьютексов
    pthread_mutex_unlock(&mutex2);
    printf("Thread 1: Mutex 2 освобожден.\n");
    pthread_mutex_unlock(&mutex1);
    printf("Thread 1: Mutex 1 освобожден.\n");

    pthread_exit(NULL);
}

// Функция, выполняемая Потоком 2
void *thread_two(void *arg) {
    printf("Thread 2: Пытаюсь захватить Mutex 2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Mutex 2 захвачен.\n");

    // Искусственная задержка, чтобы дать Потоку 1 время захватить Mutex 1
    sleep(1);

    printf("Thread 2: Пытаюсь захватить Mutex 1...\n");
    // Здесь произойдет ожидание, если Mutex 1 уже захвачен Потоком 1
    pthread_mutex_lock(&mutex1);
    printf("Thread 2: Mutex 1 захвачен. Успех!\n");

    // Критическая секция
    printf("Thread 2: Выполняю работу...\n");

    // Освобождение мьютексов
    pthread_mutex_unlock(&mutex1);
    printf("Thread 2: Mutex 1 освобожден.\n");
    pthread_mutex_unlock(&mutex2);
    printf("Thread 2: Mutex 2 освобожден.\n");

    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2;

    printf("--- Запуск демонстрации Deadlock ---\n");

    // Создание потоков
    if (pthread_create(&t1, NULL, thread_one, NULL) != 0) {
        perror("pthread_create t1");
        return 1;
    }
    if (pthread_create(&t2, NULL, thread_two, NULL) != 0) {
        perror("pthread_create t2");
        return 1;
    }

    // Ожидание завершения. В случае Deadlock, эта часть зависнет.
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("--- Программа завершена (маловероятно, если Deadlock сработал) ---\n");
    return 0;
}