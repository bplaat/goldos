#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stdbool.h>

uint16_t string_length(char *string);

bool string_equals(char *string1, char *string2);

char *string_reverse(char *string);

char *string_ltrim(char *string);

char *string_rtrim(char *string);

char *string_trim(char *string);

char *int_to_string(char *buffer, int16_t number, uint8_t base);

int16_t string_to_int(char *string);

#endif
