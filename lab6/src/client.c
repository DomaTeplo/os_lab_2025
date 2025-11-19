// client.c (Исправленная версия с libmath.h)
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "libmath.h" // <-- Подключаем нашу библиотеку

struct Server {
    char ip[255];
    int port;
};

// Используем общую структуру MathTaskArgs
struct ClientTaskArgs { 
    struct Server server;
    struct MathTaskArgs task; // Встраиваем общую структуру
};

// Функция, которую будет выполнять каждый поток для связи с сервером
void *ProcessServerTask(void *args) {
    struct ClientTaskArgs *task_args = (struct ClientTaskArgs *)args;

    // Если диапазон пуст, возвращаем нейтральный элемент (1)
    if (task_args->task.begin > task_args->task.end) {
        task_args->task.result = 1;
        return NULL;
    }
    
    struct hostent *hostname = gethostbyname(task_args->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s. Setting result to 1.\n", task_args->server.ip);
        task_args->task.result = 1; 
        return NULL;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(task_args->server.port);
    // Используем h_addr_list[0] для совместимости с gethostbyname
    memcpy(&server.sin_addr, hostname->h_addr_list[0], hostname->h_length);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed for server %s:%d! Setting result to 1.\n", task_args->server.ip, task_args->server.port);
        task_args->task.result = 1; 
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed to server %s:%d. Setting result to 1.\n", task_args->server.ip, task_args->server.port);
        close(sck);
        task_args->task.result = 1; 
        return NULL;
    }

    // Формирование задания: [begin][end][mod]
    char task_data[sizeof(uint64_t) * 3];
    memcpy(task_data, &task_args->task.begin, sizeof(uint64_t));
    memcpy(task_data + sizeof(uint64_t), &task_args->task.end, sizeof(uint64_t));
    memcpy(task_data + 2 * sizeof(uint64_t), &task_args->task.mod, sizeof(uint64_t));

    if (send(sck, task_data, sizeof(task_data), 0) < 0) {
        fprintf(stderr, "Send failed to server %s:%d. Setting result to 1.\n", task_args->server.ip, task_args->server.port);
        close(sck);
        task_args->task.result = 1; 
        return NULL;
    }

    // Получение ответа
    char response[sizeof(uint64_t)];
    int bytes_received = recv(sck, response, sizeof(response), 0);
    
    if (bytes_received < sizeof(uint64_t)) {
        fprintf(stderr, "Receive failed or incomplete from server %s:%d (received %d bytes). Setting result to 1.\n", task_args->server.ip, task_args->server.port, bytes_received);
        close(sck);
        task_args->task.result = 1; 
        return NULL;
    }

    // Сохранение ответа
    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    task_args->task.result = answer;

    close(sck);
    return NULL; 
}


int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file_path[255] = {'\0'}; 

    // --- Парсинг аргументов ---
    while (true) {

        static struct option options[] = {{"k", required_argument, 0, 0},
                                         {"mod", required_argument, 0, 0},
                                         {"servers", required_argument, 0, 0},
                                         {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                // Используем ConvertStringToUI64 из libmath
                ConvertStringToUI64(optarg, &k);
                break;
            case 1:
                // Используем ConvertStringToUI64 из libmath
                ConvertStringToUI64(optarg, &mod);
                break;
            case 2:
                memcpy(servers_file_path, optarg, strlen(optarg) < 255 ? strlen(optarg) : 254);
                servers_file_path[254] = '\0';
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == -1 || mod == -1 || !strlen(servers_file_path)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }
    
    // --- Чтение списка серверов из файла ---
    FILE *servers_file = fopen(servers_file_path, "r");
    if (!servers_file) {
        perror("Failed to open servers file");
        return 1;
    }

    unsigned int servers_num = 0;
    char line[256];
    while (fgets(line, sizeof(line), servers_file)) {
        if (strchr(line, ':') != NULL) { 
            servers_num++;
        }
    }

    if (servers_num == 0) {
        fprintf(stderr, "No servers found in the file.\n");
        fclose(servers_file);
        return 1;
    }

    struct Server *servers = (struct Server *)malloc(sizeof(struct Server) * servers_num);
    if (!servers) {
        perror("Failed to allocate memory for servers");
        fclose(servers_file);
        return 1;
    }

    rewind(servers_file); 
    unsigned int current_server_idx = 0;
    while (fgets(line, sizeof(line), servers_file) && current_server_idx < servers_num) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0'; 
        
        char *port_str = strchr(line, ':');
        if (port_str) {
            *port_str = '\0'; 
            port_str++;
            // Защищенное копирование IP
            strncpy(servers[current_server_idx].ip, line, sizeof(servers[current_server_idx].ip) - 1);
            servers[current_server_idx].ip[sizeof(servers[current_server_idx].ip) - 1] = '\0';
            servers[current_server_idx].port = atoi(port_str);
            current_server_idx++;
        }
    }
    fclose(servers_file);
    servers_num = current_server_idx; // Фактическое количество успешно прочитанных серверов
    
    if (servers_num == 0) {
        fprintf(stderr, "Failed to parse server list.\n");
        free(servers);
        return 1;
    }

    // --- Распределение работы и запуск потоков ---
    
    // Объявляем массивы после того, как узнали servers_num
    pthread_t threads[servers_num];
    struct ClientTaskArgs task_args[servers_num]; 

    // Распределение диапазона [1, k] поровну между серверами
    uint64_t chunk_size = k / servers_num;
    uint64_t current_begin = 1;
    
    for (unsigned int i = 0; i < servers_num; i++) {
        task_args[i].server = servers[i];
        // Заполнение аргументов: используем task.*
        task_args[i].task.mod = mod;
        task_args[i].task.begin = current_begin;
        
        uint64_t current_end = current_begin + chunk_size - 1;
        
        if (i == servers_num - 1) {
            current_end = k;
        }
        
        task_args[i].task.end = current_end;
        
        // Запускаем поток для каждого сервера, если есть что считать
        if (task_args[i].task.begin <= task_args[i].task.end) {
            // Инициализируем результат 1 до запуска потока
            task_args[i].task.result = 1; 
            if (pthread_create(&threads[i], NULL, ProcessServerTask, (void *)&task_args[i])) {
                fprintf(stderr, "Error: pthread_create failed for server %u! Setting result to 1.\n", i);
                task_args[i].task.result = 1; 
            }
        } else {
            // Если диапазон пуст, результат 1
            task_args[i].task.result = 1;
        }
        
        current_begin = current_end + 1;
    }

    // --- Объединение результатов ---
    uint64_t final_answer = 1;
    for (unsigned int i = 0; i < servers_num; i++) {
        // Ждем завершения потока только если он был создан
        if (task_args[i].task.begin <= task_args[i].task.end) {
            pthread_join(threads[i], NULL);
        }
        
        // Объединяем результаты: используем MultModulo из libmath
        final_answer = MultModulo(final_answer, task_args[i].task.result, mod);
    }

    printf("Final answer for %llu! %% %llu is: %llu\n", (unsigned long long)k, (unsigned long long)mod, (unsigned long long)final_answer);

    free(servers);

    return 0;
}