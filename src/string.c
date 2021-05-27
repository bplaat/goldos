#include "string.h"

uint16_t string_length(char *string) {
    char *begin = string;
    while (*string != '\0') {
        string++;
    }
    return string - begin;
}

bool string_equals(char *string1, char *string2) {
    while (*string1 != '\0') {
        if (*string1 != *string2) {
            return false;
        }
        string1++;
        string2++;
    }
    return *string2 == '\0';
}

char *string_reverse(char *string) {
    char *begin = string;
    char *end = string + string_length(string) - 1;
    while (end > begin) {
        char tmp = *begin;
        *begin = *end;
        *end = tmp;
        begin++;
        end--;
    }
    return string;
}

char *string_ltrim(char *string) {
    while (*string == ' ') string++;
    return string;
}

char *string_rtrim(char *string) {
    char *end = string + string_length(string) - 1;
    while (*end == ' ') end--;
    *(end + 1) = '\0';
    return string;
}

char *string_trim(char *string) {
    return string_ltrim(string_rtrim(string));
}

char *int_to_string(char *buffer, int16_t number, uint8_t base) {
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    int16_t value = number > 0 ? number : -number;

    char *begin = buffer;
    while (value != 0) {
        int16_t digit = value % base;
        if (digit >= 10) {
            *buffer++ = 'A' + (digit - 10);
        } else {
            *buffer++ = '0' + digit;
        }
        value = value / base;
    }

    if (buffer == begin) {
        *buffer++ = '0';
    }
    if (number < 0 && base == 10) {
        *buffer++ = '-';
    }
    *buffer = '\0';

    return string_reverse(begin);
}

int16_t string_to_int(char *string) {
    bool isNegative = false;
    if (*string == '-') {
        isNegative = true;
        string++;
    }

    int16_t number = 0;
    while (*string != '\0') {
        if (*string >= '0' && *string <= '9') {
            number = number * 10 + (*string - '0'); // ((number << 3) + (number << 1))
        }
        string++;
    }

    if (isNegative) {
        number = -number;
    }
    return number;
}
