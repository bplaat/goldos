#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define DISK_BLOCK_ALIGN 8

uint16_t disk_alloc(uint16_t size);

void disk_free(uint16_t position);

uint16_t disk_realloc(uint16_t position, uint16_t size);

#endif
