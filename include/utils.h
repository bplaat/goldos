#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

typedef union FloatConvert {
    float number;
    uint32_t dword;
} FloatConvert;

uint16_t align(uint16_t number, uint16_t alignment);

uint16_t rand_int(uint16_t min, uint16_t max);

#endif
