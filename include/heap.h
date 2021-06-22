#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

#define HEAP_BLOCK_ALIGN 2

#define HEAP_SIZE 64

extern uint8_t heap[];

void heap_begin();

uint8_t heap_alloc(uint8_t size);

void heap_free(uint8_t address);

uint8_t heap_find(uint8_t id);

void heap_set_byte(uint8_t id, uint8_t byte);

void heap_set_word(uint8_t id, uint16_t word);

void heap_set_dword(uint8_t id, uint32_t dword);

void heap_set_float(uint8_t id, float number);

void heap_set_string(uint8_t id, char *string);

uint8_t *heap_get_byte(uint8_t id);

uint16_t *heap_get_word(uint8_t id);

uint32_t *heap_get_dword(uint8_t id);

float *heap_get_float(uint8_t id);

char *heap_get_string(uint8_t id);

void heap_clear(uint8_t id);

void heap_inspect(void);

#endif
