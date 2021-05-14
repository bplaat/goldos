#include "utils.h"

uint16_t string_length(uint8_t* string) {
    uint8_t* begin = string;
    while (*string != '\0') {
        string++;
    }
    return string - begin;
}

bool string_equals(uint8_t* string1, uint8_t* string2) {
    while (*string1 != '\0') {
        if (*string1 != *string2) {
            return false;
        }
        string1++;
        string2++;
    }
    return *string2 == '\0';
}

uint8_t* string_reverse(uint8_t* string) {
    uint8_t* begin = string;
    uint8_t* end = string + string_length(string) - 1;
    while (end > begin) {
        *begin ^= *end;
        *end ^= *begin;
        *begin ^= *end;
        begin++;
        end--;
    }
    return string;
}

uint8_t* string_ltrim(uint8_t* string) {
    while (*string == ' ') string++;
    return string;
}

uint8_t* string_rtrim(uint8_t* string) {
    uint8_t* end = string + string_length(string) - 1;
    while (*end == ' ') end--;
    *(end + 1) = '\0';
    return string;
}

uint8_t* string_trim(uint8_t* string) {
    return string_ltrim(string_rtrim(string));
}

uint8_t* int_to_string(uint8_t* buffer, int16_t number, uint8_t base) {
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    int16_t value = number > 0 ? number : -number;

    uint8_t i = 0;
    while (value != 0) {
        int16_t digit = value % base;
        if (digit >= 10) {
            buffer[i++] = 'A' + (digit - 10);
        } else {
            buffer[i++] = '0' + digit;
        }
        value = value / base;
    }

    if (i == 0) {
        buffer[i++] = '0';
    }

    if (number < 0 && base == 10) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    return string_reverse(buffer);
}

int16_t string_to_int(uint8_t* string) {
    bool isNegative = false;
    if (*string == '-') {
        isNegative = true;
        string++;
    }

    int16_t number = 0;
    while (*string != '\0') {
        if (*string >= '0' && *string <= '9') {
            number = ((number << 3) + (number << 1)) + (*string - '0');
        }
        string++;
    }
    if (isNegative) {
        number = -number;
    }
    return number;
}
