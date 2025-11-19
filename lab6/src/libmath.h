// libmath.h
#ifndef LIBMATH_H
#define LIBMATH_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

// Общая структура для аргументов факториала/диапазона
// Может использоваться и клиентом, и сервером
struct MathTaskArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result; 
};

// Прототипы функций
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif // LIBMATH_H