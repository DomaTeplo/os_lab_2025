// libmath.c
#include "libmath.h"

// 1. Модульное умножение
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }

    return result % mod;
}

// 2. Конвертация строки в uint64_t
bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    // strtoull требует <stdlib.h>
    unsigned long long i = strtoull(str, &end, 10); 
    
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }

    // Проверка, что вся строка была обработана и нет ошибки
    if (errno != 0 || *end != '\0') 
        return false;

    *val = i;
    return true;
}