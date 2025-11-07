#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>

#include "utils.h"
#include "sum_utils.h"  // ← УЖЕ ВКЛЮЧАЕТ struct SumArgs

// ⚠️ УДАЛИЛ структуру SumArgs отсюда - она теперь в sum_utils.h

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Парсинг аргументов командной строки
  static struct option options[] = {
      {"threads_num", required_argument, 0, 0},
      {"seed", required_argument, 0, 0},
      {"array_size", required_argument, 0, 0},
      {0, 0, 0, 0}
  };

  int option_index = 0;
  while (true) {
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: // --threads_num
            threads_num = atoi(optarg);
            if (threads_num <= 0) {
              printf("threads_num must be positive\n");
              return 1;
            }
            break;
          case 1: // --seed
            seed = atoi(optarg);
            break;
          case 2: // --array_size
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
        }
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (threads_num == 0 || array_size == 0) {
    printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
    return 1;
  }

  // Ограничиваем число потоков если массив слишком маленький
  if (threads_num > array_size) {
    threads_num = array_size;
  }

  // Генерация массива (НЕ входит в замер времени)
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // Начало замера времени
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  // Вычисление размеров частей массива
  int chunk_size = array_size / threads_num;
  int remainder = array_size % threads_num;

  // Создание потоков
  for (uint32_t i = 0; i < threads_num; i++) {
    // Вычисление границ для текущего потока
    args[i].array = array;
    args[i].begin = i * chunk_size + (i < remainder ? i : remainder);
    args[i].end = args[i].begin + chunk_size + (i < remainder ? 1 : 0);

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  // Ожидание завершения потоков и сбор результатов
  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    long long sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  // Конец замера времени
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}