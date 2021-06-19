#include "stack.h"
#include <string.h>
#include "serial.h"

uint8_t stack[STACK_SIZE];

uint8_t stack_pointer = STACK_SIZE - 1;

uint8_t stack_pop_byte(void) {
    return stack[++stack_pointer];
}

uint16_t stack_pop_word(void) {
    uint16_t data = stack[stack_pointer + 1] | (stack[stack_pointer + 2] << 8);
    stack_pointer += 2;
    return data;
}

uint32_t stack_pop_dword(void) {
    uint32_t data = stack[stack_pointer + 1] | (stack[stack_pointer + 2] << 8) | ((uint32_t)stack[stack_pointer + 3] << 16) | ((uint32_t)stack[stack_pointer + 4] << 24);
    stack_pointer += 4;
    return data;
}

float stack_pop_float(void) {
    FloatConvert convert;
    convert.data = stack[stack_pointer + 1] | (stack[stack_pointer + 2] << 8) | ((uint32_t)stack[stack_pointer + 3] << 16) | ((uint32_t)stack[stack_pointer + 4] << 24);
    stack_pointer += 4;
    return convert.number;
}

void stack_pop_string(char *string) {
    uint8_t size = stack[++stack_pointer];
    for (uint8_t i = 0; i < size; i++) {
        string[i] = stack[++stack_pointer];
    }
}

void stack_push_byte(uint8_t data) {
    stack[stack_pointer--] = data;
}

void stack_push_word(uint16_t data) {
    stack[stack_pointer--] = data >> 8;
    stack[stack_pointer--] = data & 0xff;
}

void stack_push_dword(uint32_t data) {
    stack[stack_pointer--] = data >> 24;
    stack[stack_pointer--] = (data >> 16) & 0xff;
    stack[stack_pointer--] = (data >> 8) & 0xff;
    stack[stack_pointer--] = data & 0xff;
}

void stack_push_float(float number) {
    FloatConvert convert;
    convert.number = number;
    stack[stack_pointer--] = convert.data >> 24;
    stack[stack_pointer--] = (convert.data >> 16) & 0xff;
    stack[stack_pointer--] = (convert.data >> 8) & 0xff;
    stack[stack_pointer--] = convert.data & 0xff;
}

void stack_push_string(char *string) {
    uint8_t size = strlen(string);
    for (int16_t i = size - 1; i >= 0; i--) {
        stack[stack_pointer--] = string[i];
    }
    stack[stack_pointer--] = size;
}

void stack_inspect(void) {
    serial_println_P(PSTR("Stack:"));
    serial_print_P(PSTR("stack_pointer = "));
    serial_print_byte(stack_pointer, '0');
    serial_write('\n');

    if (stack_pointer != STACK_SIZE - 1) {
        for (uint8_t i = STACK_SIZE - 1; i > stack_pointer; i--) {
            serial_print_byte(i, '0');
            serial_print_P(PSTR(": "));
            serial_print_byte(stack[i], '0');
            serial_write('\n');
        }
    } else {
        serial_println_P(PSTR("The stack is empty"));
    }
}
