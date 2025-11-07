#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Константы для Задания 2 (файлы)
#define FILENAME_FORMAT "temp_minmax_%d.txt"
#define MAX_FILENAME_LEN 64

// Глобальные переменные для обработки сигналов
volatile sig_atomic_t timeout_occurred = 0;
volatile sig_atomic_t active_child_processes = 0;
pid_t* child_pids = NULL;

// Обработчик сигнала SIGALRM
void handle_alarm(int sig) {
    timeout_occurred = 1;
    
    // Посылаем SIGKILL всем дочерним процессам
    for (int i = 0; i < active_child_processes; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
    // -------------------------------------------------------------------------
    // БЛОК 1: Инициализация и парсинг аргументов командной строки
    // -------------------------------------------------------------------------
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0; // 0 означает отсутствие таймаута
    bool with_files = false;
    
    // Переменная для хранения файловых дескрипторов pipe (для Задания 3). 
    // Будет инициализирована при pnum > 0 и with_files = false.
    int (*pipe_fds)[2] = NULL; 

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"timeout", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: // --seed
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            fprintf(stderr, "seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1: // --array_size
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            fprintf(stderr, "array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2: // --pnum
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            fprintf(stderr, "pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 3: // --timeout
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            fprintf(stderr, "timeout must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 4: // --by_files
                        with_files = true;
                        break;
                    defalut:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"] [--by_files]\n",
               argv[0]);
        return 1;
    }
    
    // Ограничиваем число процессов, чтобы не делить пустые массивы
    if (pnum > array_size) {
        pnum = array_size;
    }
    
    // -------------------------------------------------------------------------
    // БЛОК 2: Подготовка данных и ресурсов
    // -------------------------------------------------------------------------
    
    // Выделяем память под массив и генерируем числа
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        perror("Failed to allocate array");
        return 1;
    }
    GenerateArray(array, array_size, seed);
    
    // Инициализация массива для хранения PID дочерних процессов
    child_pids = malloc(sizeof(pid_t) * pnum);
    if (child_pids == NULL) {
        perror("Failed to allocate child_pids");
        free(array);
        return 1;
    }
    for (int i = 0; i < pnum; i++) {
        child_pids[i] = 0;
    }
    
    active_child_processes = 0;
    
    // Подготовка pipe'ов (для Задания 3)
    if (!with_files) {
        pipe_fds = malloc(sizeof(int[2]) * pnum);
        if (pipe_fds == NULL) {
            perror("Failed to allocate pipe_fds");
            free(array);
            free(child_pids);
            return 1;
        }
        // Создаем все pipe до запуска fork
        for(int i = 0; i < pnum; ++i) {
            if (pipe(pipe_fds[i]) == -1) {
                perror("Failed to create pipe");
                free(array);
                free(child_pids);
                free(pipe_fds);
                return 1;
            }
        }
    }

    // Установка обработчика сигнала SIGALRM
    if (timeout > 0) {
        if (signal(SIGALRM, handle_alarm) == SIG_ERR) {
            perror("Failed to set signal handler");
            free(array);
            free(child_pids);
            if (!with_files) free(pipe_fds);
            return 1;
        }
        // Устанавливаем таймер
        alarm(timeout);
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Вычисляем размеры частей массива для равномерного распределения
    int chunk_size = array_size / pnum;
    int remainder = array_size % pnum;

    // -------------------------------------------------------------------------
    // БЛОК 3: Создание дочерних процессов (fork)
    // -------------------------------------------------------------------------
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        
        if (child_pid >= 0) {
            // successful fork
            active_child_processes += 1;
            child_pids[i] = child_pid;
            
            if (child_pid == 0) {
                // *** CHILD PROCESS (ДОЧЕРНИЙ ПРОЦЕСС) ***
                
                // Вычисление границ для текущей части массива
                unsigned int begin = i * chunk_size + (i < remainder ? i : remainder);
                unsigned int current_chunk_size = chunk_size + (i < remainder ? 1 : 0);
                unsigned int end = begin + current_chunk_size;
                
                // Поиск локального MinMax
                struct MinMax local_min_max = GetMinMax(array, begin, end);

                if (with_files) {
                    // *** ЗАДАНИЕ 2: Использование файлов (Запись) ***
                    char filename[MAX_FILENAME_LEN];
                    sprintf(filename, FILENAME_FORMAT, i);
                    
                    FILE *fp = fopen(filename, "w");
                    if (fp == NULL) {
                        perror("Child failed to open file");
                        free(array);
                        exit(1);
                    }
                    // Запись min и max в файл
                    fprintf(fp, "%d %d", local_min_max.min, local_min_max.max);
                    fclose(fp);
                    
                } else {
                    // *** ЗАДАНИЕ 3: Использование pipe (Запись) ***
                    // Закрываем неиспользуемый конец pipe (чтение)
                    close(pipe_fds[i][0]);
                    
                    // Отправляем min и max через pipe
                    write(pipe_fds[i][1], &local_min_max.min, sizeof(int));
                    write(pipe_fds[i][1], &local_min_max.max, sizeof(int));
                    
                    // Закрываем используемый конец pipe (запись)
                    close(pipe_fds[i][1]);
                }
                
                free(array); // Освобождаем память массива
                if (!with_files) free(pipe_fds); // Освобождаем память дескрипторов pipe
                free(child_pids); // Освобождаем память массива PID
                exit(0); // Завершение дочернего процесса
            }

            // *** PARENT PROCESS (РОДИТЕЛЬСКИЙ ПРОЦЕСС) ***
            if (!with_files) {
                // Если используем pipe, родитель закрывает неиспользуемый конец (запись)
                close(pipe_fds[i][1]);
            }

        } else {
            perror("Fork failed!");
            free(array);
            free(child_pids);
            if (!with_files) free(pipe_fds);
            return 1;
        }
    }

    // -------------------------------------------------------------------------
    // БЛОК 4: Ожидание завершения дочерних процессов с поддержкой таймаута
    // -------------------------------------------------------------------------
    
    int remaining_children = active_child_processes;
    
    while (remaining_children > 0) {
        int status;
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);
        
        if (finished_pid > 0) {
            // Дочерний процесс завершился
            remaining_children--;
            
            // Находим и обнуляем PID завершенного процесса в массиве
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] == finished_pid) {
                    child_pids[i] = 0;
                    break;
                }
            }
        } else if (finished_pid == 0) {
            // Нет завершенных процессов, проверяем таймаут
            if (timeout_occurred) {
                printf("Timeout occurred! Sending SIGKILL to remaining child processes.\n");
                break;
            }
            // Небольшая пауза перед следующей проверкой
            usleep(10000); // 10ms
        } else {
            // Ошибка в waitpid
            if (errno != ECHILD) { // ECHILD - нет дочерних процессов (нормально)
                perror("waitpid failed");
            }
            break;
        }
    }

    // Отменяем таймер, если он еще не сработал
    if (timeout > 0) {
        alarm(0);
    }

    // -------------------------------------------------------------------------
    // БЛОК 5: Сбор результатов и поиск глобального MinMax
    // -------------------------------------------------------------------------
    
    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    int successful_results = 0;
    
    for (int i = 0; i < pnum; i++) {
        // Пропускаем процессы, которые были убиты по таймауту
        if (timeout_occurred && child_pids[i] != 0) {
            continue;
        }
        
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            // *** ЗАДАНИЕ 2: Чтение из файлов ***
            char filename[MAX_FILENAME_LEN];
            sprintf(filename, FILENAME_FORMAT, i);
            
            FILE *fp = fopen(filename, "r");
            if (fp == NULL) {
                fprintf(stderr, "Parent failed to open file %s for reading. Skipping result.\n", filename);
                continue;
            }
            
            // Считывание двух чисел
            if (fscanf(fp, "%d %d", &min, &max) != 2) {
                fprintf(stderr, "Error reading MinMax from file %s. Skipping result.\n", filename);
            } else {
                successful_results++;
            }
            
            fclose(fp);
            remove(filename); // Удаление временного файла
            
        } else {
            // *** ЗАДАНИЕ 3: Чтение из pipe ***
            
            // Читаем min, затем max
            if (read(pipe_fds[i][0], &min, sizeof(int)) != sizeof(int) ||
                read(pipe_fds[i][0], &max, sizeof(int)) != sizeof(int)) {
                perror("Parent failed to read from pipe. Skipping result.");
                min = INT_MAX; 
                max = INT_MIN;
            } else {
                successful_results++;
            }
            
            // Закрываем pipe (чтение)
            close(pipe_fds[i][0]); 
        }

        // Обновление глобального MinMax
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }
    
    // Освобождение памяти для файловых дескрипторов pipe
    if (!with_files) {
        free(pipe_fds);
    }
    
    free(child_pids);

    // -------------------------------------------------------------------------
    // БЛОК 6: Измерение времени и вывод результата
    // -------------------------------------------------------------------------
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    
    if (timeout_occurred) {
        printf("Warning: Timeout occurred! Only %d out of %d processes completed successfully.\n", 
               successful_results, pnum);
    } else {
        printf("All %d processes completed successfully.\n", pnum);
    }
    
    fflush(NULL);
    return 0;
}