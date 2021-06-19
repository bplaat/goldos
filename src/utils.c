#include "utils.h"
#include <stdlib.h>

uint16_t align(uint16_t number, uint16_t alignment) {
    uint16_t reminder = number % alignment;
    return reminder ? number + alignment - reminder : number;
}

uint16_t rand_int(uint16_t min, uint16_t max) {
    return rand() % ((max - min) + 1) + min;
}
