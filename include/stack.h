#ifndef STACK_H
#define STACK_H

#include <stdint.h>

typedef union FloatConvert {
    float number;
    uint32_t data;
} FloatConvert;

#define STACK_SIZE 255

extern uint8_t stack[];

extern uint8_t stack_pointer;

uint8_t stack_pop_byte(void);

uint16_t stack_pop_word(void);

uint32_t stack_pop_dword(void);

float stack_pop_float(void);

void stack_pop_string(char *string);

void stack_push_byte(uint8_t data);

void stack_push_word(uint16_t data);

void stack_push_dword(uint32_t data);

void stack_push_float(float number);

void stack_push_string(char *string);

void stack_inspect(void);

#endif
