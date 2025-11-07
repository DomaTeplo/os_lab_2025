#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

void sigchld_handler(int sig) {
    printf("[Handler] Получен SIGCHLD\n");
    
    // Обрабатываем все завершившиеся процессы
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[Handler] Зомби-процесс %d убран\n", pid);
    }
}

int main() {
    printf("=== Расширенная демонстрация зомби-процессов ===\n\n");
    
    char choice;
    printf("Выберите действие ДО создания процессов:\n");
    printf("   a) Создать зомби (не обрабатывать SIGCHLD)\n");
    printf("   b) Обработать с помощью обработчика сигнала\n");
    printf("   c) Игнорировать SIGCHLD\n");
    printf("Ваш выбор: ");
    scanf(" %c", &choice);
    
    // Устанавливаем обработку сигналов ДО создания процессов
    switch (choice) {
        case 'a':
            printf("\n--- РЕЖИМ ЗОМБИ ---\n");
            printf("Не устанавливаем обработчик SIGCHLD\n");
            // Оставляем обработчик по умолчанию
            break;
            
        case 'b':
            printf("\n--- РЕЖИМ ОБРАБОТЧИКА ---\n");
            // Устанавливаем обработчик SIGCHLD ДО fork()
            if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) {
                perror("Ошибка установки обработчика");
                return 1;
            }
            printf("Обработчик SIGCHLD установлен\n");
            break;
            
        case 'c':
            printf("\n--- РЕЖИМ ИГНОРИРОВАНИЯ ---\n");
            // Игнорируем SIGCHLD ДО fork()
            if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
                perror("Ошибка установки игнорирования");
                return 1;
            }
            printf("SIGCHLD игнорируется\n");
            break;
            
        default:
            printf("Неверный выбор!\n");
            return 1;
    }
    
    printf("\n1. Создаем несколько дочерних процессов...\n");
    
    // Сохраняем PID дочерних процессов
    pid_t child_pids[3];
    
    // Создаем 3 дочерних процесса ПОСЛЕ установки обработчика
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Ошибка fork");
            return 1;
        }
        
        if (pid == 0) {
            // Дочерний процесс
            printf("[Child%d] PID: %d - запущен\n", i, getpid());
            sleep(1); // Работаем 1 секунду
            printf("[Child%d] PID: %d - завершается\n", i, getpid());
            exit(i);
        } else {
            child_pids[i] = pid;
            printf("[Parent] Создан дочерний процесс PID: %d\n", pid);
        }
    }
    
    printf("\n2. Дочерние процессы созданы. Ожидаем их завершения...\n");
    
    // В зависимости от режима, по-разному ждем завершения
    switch (choice) {
        case 'a':
            printf("Дочерние процессы станут зомби на 10 секунд...\n");
            printf("Проверьте зомби в другом терминале командой: ps aux | grep defunct\n");
            sleep(10);
            printf("Теперь убираем зомби вручную...\n");
            
            // БЛОКИРУЮЩЕЕ ожидание - гарантированно ждем всех детей
            for (int i = 0; i < 3; i++) {
                int status;
                pid_t finished_pid = waitpid(child_pids[i], &status, 0);
                if (finished_pid > 0) {
                    printf("Зомби %d убран, статус: %d\n", finished_pid, WEXITSTATUS(status));
                }
            }
            break;
            
        case 'b':
            printf("Ожидаем завершения дочерних процессов...\n");
            printf("Обработчик SIGCHLD автоматически уберет зомби\n");
            sleep(5); // Даем время для срабатывания обработчика
            break;
            
        case 'c':
            printf("Ожидаем завершения дочерних процессов...\n");
            printf("Зомби не появятся благодаря игнорированию SIGCHLD\n");
            sleep(5);
            break;
    }
    
    // Финальная проверка - неблокирующая, на случай если что-то осталось
    printf("\n3. Финальная очистка...\n");
    int status;
    pid_t pid;
    int zombies_cleaned = 0;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Финальная очистка: зомби %d убран\n", pid);
        zombies_cleaned++;
    }
    
    if (zombies_cleaned == 0) {
        printf("Все зомби-процессы убраны\n");
    }
    
    printf("\nПрограмма завершена.\n");
    
    // Короткая пауза чтобы успеть проверить
    if (choice == 'a') {
        printf("Проверьте что зомби исчезли\n");
        sleep(5);
    }

    // Короткая пауза чтобы успеть проверить
    if (choice == 'b') {
        printf("Проверьте что зомби исчезли\n");
        sleep(5);
    }

    // Короткая пауза чтобы успеть проверить
    if (choice == 'c') {
        printf("Проверьте что зомби исчезли\n");
        sleep(5);
    }
    
    return 0;
}