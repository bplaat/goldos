#include "heap.h"
#include <stdbool.h>
#include <string.h>
#include "utils.h"
#include "serial.h"

uint8_t heap[HEAP_SIZE];

void heap_begin(void) {
    #ifdef DEBUG
        for (uint16_t i = 0; i < HEAP_SIZE; i++) {
            heap[i] = 0;
        }
    #endif

    heap[0] = 'B';
    heap[1] = 'P';

    uint8_t free_block_size = HEAP_SIZE - HEAP_BLOCK_ALIGN - 2;
    heap[HEAP_BLOCK_ALIGN] = free_block_size;
    heap[HEAP_SIZE - 1] = free_block_size;
}

uint8_t heap_alloc(uint8_t size) {
    size = align(size != 0 ? size : 1, HEAP_BLOCK_ALIGN);
    uint8_t block_address = HEAP_BLOCK_ALIGN;
    while (block_address <= HEAP_SIZE - 1 - 1) {
        uint8_t block_header = heap[block_address];
        uint8_t block_size = block_header & 0x7f;
        if ((block_header & 0x80) == 0 && (block_size == size || block_size >= size + 1 + 1)) {
            heap[block_address] = 0x80 | size;
            heap[block_address + 1 + size] = 0x80 | size;
            if (block_size != size) {
                uint8_t new_next_block_address = block_address + 1 + size + 1;
                uint8_t new_next_block_size = block_size - 1 - size- 1;
                heap[new_next_block_address] = new_next_block_size;
                heap[new_next_block_address + 1 + new_next_block_size] = new_next_block_size;
            }
            return block_address + 1;
        }
        block_address += 1 + block_size + 1;
    }
    return 0;
}

void heap_free(uint8_t address) {
    if (address == 0) return;
    uint8_t block_address = address - 1;
    uint8_t block_header = heap[block_address];
    if ((block_header & 0x80) == 0) return;
    uint8_t block_size = block_header & 0x7f;

    if (block_address - 1 >= HEAP_BLOCK_ALIGN + 1) {
        uint8_t previous_block_header = heap[block_address - 1];
        if ((previous_block_header & 0x80) == 0) {
            uint8_t previous_block_size = previous_block_header & 0x7f;
            block_address -= 1 + previous_block_size + 1;
            block_size += 1 + previous_block_size + 1;
        }
    }

    if (block_address + 1 + block_size + 1 <= HEAP_SIZE - 1 - 1) {
        uint8_t next_block_header = heap[block_address + 1 + block_size + 1];
        if ((next_block_header & 0x80) == 0) {
            uint8_t next_block_size = next_block_header & 0x7f;
            block_size += 1 + next_block_size + 1;
        }
    }

    heap[block_address] = block_size;
    heap[block_address + 1 + block_size] = block_size;
}

uint8_t heap_find(uint8_t id) {
    uint8_t block_address = HEAP_BLOCK_ALIGN;
    while (block_address <= HEAP_SIZE - 1) {
        uint8_t block_header = heap[block_address];
        uint8_t block_size = block_header & 0x7f;
        if ((block_header & 0x80) != 0) {
            if (heap[block_address + 1] == id) {
                return block_address + 1;
            }
        }
        block_address += 1 + block_size + 1;
    }
    return 0;
}

void heap_set_byte(uint8_t id, uint8_t byte) {
    uint8_t old_address = heap_find(id);
    if (old_address != 0) heap_free(old_address);

    uint8_t address = heap_alloc(1 + sizeof(uint8_t));
    if (address != 0) {
        heap[address] = id;
        heap[address + 1] = byte;
    }
}

void heap_set_word(uint8_t id, uint16_t word) {
    uint8_t old_address = heap_find(id);
    if (old_address != 0) heap_free(old_address);

    uint8_t address = heap_alloc(1 + sizeof(uint16_t));
    if (address != 0) {
        heap[address] = id;
        heap[address + 1] = word & 0xff;
        heap[address + 2] = word >> 8;
    }
}

void heap_set_dword(uint8_t id, uint32_t dword) {
    uint8_t old_address = heap_find(id);
    if (old_address != 0) heap_free(old_address);

    uint8_t address = heap_alloc(1 + sizeof(uint32_t));
    if (address != 0) {
        heap[address] = id;
        heap[address + 1] = dword & 0xff;
        heap[address + 2] = (dword & 0xff) >> 8;
        heap[address + 3] = (dword & 0xff) >> 16;
        heap[address + 4] = dword >> 24;
    }
}

void heap_set_float(uint8_t id, float number) {
    FloatConvert convert;
    convert.number = number;
    heap_set_dword(id, convert.dword);
}

void heap_set_string(uint8_t id, char *string) {
    uint8_t old_address = heap_find(id);
    if (old_address != 0) heap_free(old_address);

    uint8_t size = strlen(string);
    uint8_t address = heap_alloc(1 + size + 1);
    if (address != 0) {
        heap[address] = id;
        for (uint8_t i = 0; i < size; i++) {
            heap[address + 1 + i] = string[i];
        }
        heap[address + 1 + size] = '\0';
    }
}

uint8_t *heap_get_byte(uint8_t id) {
    uint8_t address = heap_find(id);
    if (address != 0) {
        return &heap[address + 1];
    } else {
        return NULL;
    }
}

uint16_t *heap_get_word(uint8_t id) {
    uint8_t address = heap_find(id);
    if (address != 0) {
        return (uint16_t *)&heap[address + 1];
    } else {
        return NULL;
    }
}

uint32_t *heap_get_dword(uint8_t id) {
    uint8_t address = heap_find(id);
    if (address != 0) {
        return (uint32_t *)&heap[address + 1];
    } else {
        return NULL;
    }
}

float *heap_get_float(uint8_t id) {
    return (float *)heap_get_dword(id);
}

char *heap_get_string(uint8_t id) {
    uint8_t address = heap_find(id);
    if (address != 0) {
        return (char *)&heap[address + 1];
    } else {
        return NULL;
    }
}

void heap_inspect(void) {
    serial_println_P(PSTR("Heap blocks:"));

    uint8_t block_address = HEAP_BLOCK_ALIGN;
    uint8_t free_block_count = 0;
    uint8_t free_blocks_size = 0;
    uint8_t max_free_block_size = 0;
    while (block_address <= HEAP_SIZE - 1 - 1) {
        uint8_t block_header = heap[block_address];
        uint8_t block_size = block_header & 0x7f;

        serial_print_P(PSTR("- "));
        if ((block_header & 0x80) == 0) {
            serial_print_P(PSTR("Free block of "));
            serial_print_number(block_size, '\0');
            serial_println_P(PSTR(" bytes"));

            free_block_count++;
            free_blocks_size += block_size;
            if (block_size > max_free_block_size) {
                max_free_block_size = block_size;
            }
        } else {
            serial_print_P(PSTR("Allocated block with id '"));
            serial_print_byte(heap[block_address + 1], '0');
            serial_print_P(PSTR("' of "));
            serial_print_number(block_size, '\0');
            serial_println_P(PSTR(" bytes"));
        }
        block_address += 1 + block_size + 1;
    }

    serial_print_P(PSTR("\nFree blocks size is "));
    serial_print_number(free_blocks_size, '\0');
    serial_print_P(PSTR(" bytes\nSeperated over "));
    serial_print_number(free_block_count, '\0');
    serial_print_P(PSTR(" free blocks\nLargest free block is "));
    serial_print_number(max_free_block_size, '\0');
    serial_println_P(PSTR(" bytes"));
}
