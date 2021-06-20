#include "stack.h"
#include <string.h>
#include "utils.h"
#include "serial.h"

uint8_t stack[STACK_SIZE];

uint8_t stack_pointer = STACK_SIZE - 1;

uint8_t stack_pop_byte(void) {
    return stack[++stack_pointer];
}

uint16_t stack_pop_word(void) {
    uint16_t word = stack[stack_pointer + 1] | (stack[stack_pointer + 2] << 8);
    stack_pointer += 2;
    return word;
}

uint32_t stack_pop_dword(void) {
    uint32_t dword = stack[stack_pointer + 1] | (stack[stack_pointer + 2] << 8) | ((uint32_t)stack[stack_pointer + 3] << 16) | ((uint32_t)stack[stack_pointer + 4] << 24);
    stack_pointer += 4;
    return dword;
}

float stack_pop_float(void) {
    FloatConvert convert;
    convert.dword = stack_pop_dword();
    return convert.number;
}

void stack_pop_string(char *string) {
    uint8_t size = stack[++stack_pointer];
    for (uint8_t i = 0; i < size; i++) {
        string[i] = stack[++stack_pointer];
    }
    string[size] = '\0';
}

void stack_push_byte(uint8_t byte) {
    stack[stack_pointer--] = byte;
}

void stack_push_word(uint16_t word) {
    stack[stack_pointer--] = word >> 8;
    stack[stack_pointer--] = word & 0xff;
}

void stack_push_dword(uint32_t dword) {
    stack[stack_pointer--] = dword >> 24;
    stack[stack_pointer--] = (dword >> 16) & 0xff;
    stack[stack_pointer--] = (dword >> 8) & 0xff;
    stack[stack_pointer--] = dword & 0xff;
}

void stack_push_float(float number) {
    FloatConvert convert;
    convert.number = number;
    stack_push_dword(convert.dword);
}

void stack_push_string(char *string) {
    uint8_t size = strlen(string);
    for (int16_t i = size - 1; i >= 0; i--) {
        stack[stack_pointer--] = string[i];
    }
    stack[stack_pointer--] = size;
}

void stack_clear(void) {
    #ifdef DEBUG
        for (uint16_t i = 0; i < STACK_SIZE; i++) {
            stack[i] = 0;
        }
    #endif

    stack_pointer = STACK_SIZE - 1;
}

void stack_inspect(void) {
    serial_print_P(PSTR("Stack ("));
    serial_print_byte(stack_pointer, '0');
    serial_print_P(PSTR(" / "));
    serial_print_byte(STACK_SIZE - 1, '0');
    serial_println_P(PSTR("):"));

    if (stack_pointer != STACK_SIZE - 1) {
        for (uint8_t i = STACK_SIZE - 1; i > stack_pointer; i--) {
            serial_print_byte(i, '0');
            serial_print_P(PSTR(": "));
            serial_print_byte(stack[i], '0');
            serial_write('\n');
        }
    } else {
        serial_println_P(PSTR("- The stack is empty"));
    }
}
