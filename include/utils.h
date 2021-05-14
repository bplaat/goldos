#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

uint16_t string_length(uint8_t* string);

bool string_equals(uint8_t* string1, uint8_t* string2);

uint8_t* string_reverse(uint8_t* string);

uint8_t* string_ltrim(uint8_t* string);

uint8_t* string_rtrim(uint8_t* string);

uint8_t* string_trim(uint8_t* string);

uint8_t* int_to_string(uint8_t* buffer, int16_t number, uint8_t base);

int16_t string_to_int(uint8_t* string);

#endif
