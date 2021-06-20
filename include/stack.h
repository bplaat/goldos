#ifndef STACK_H
#define STACK_H

#include <stdint.h>

#define STACK_SIZE 64

extern uint8_t stack[];

extern uint8_t stack_pointer;

uint8_t stack_pop_byte(void);

uint16_t stack_pop_word(void);

uint32_t stack_pop_dword(void);

float stack_pop_float(void);

void stack_pop_string(char *string);

void stack_push_byte(uint8_t byte);

void stack_push_word(uint16_t word);

void stack_push_dword(uint32_t dword);

void stack_push_float(float number);

void stack_push_string(char *string);

void stack_clear(void);

void stack_inspect(void);

#endif
