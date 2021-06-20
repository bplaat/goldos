#include "disk.h"
// #include <stdio.h>
#include <stdbool.h>
#include "eeprom.h"
#include "utils.h"
#include "serial.h"

uint16_t disk_alloc(uint16_t size) {
    size = align(size > 0 ? size : 1, DISK_BLOCK_ALIGN);
    uint16_t block_address = DISK_BLOCK_ALIGN;
    while (block_address <= EEPROM_SIZE - 2 - 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) == 0 && (block_size == size || block_size >= size + 2 + 2)) {
            eeprom_write_word(block_address, 0x8000 | size);
            eeprom_write_word(block_address + 2 + size, 0x8000 | size);
            if (block_size != size) {
                uint16_t new_next_block_address = block_address + 2 + size + 2;
                uint16_t new_next_block_size = block_size - 2 - size - 2;
                eeprom_write_word(new_next_block_address, new_next_block_size);
                eeprom_write_word(new_next_block_address + 2 + new_next_block_size, new_next_block_size);
            }
            return block_address + 2;
        }
        block_address += 2 + block_size + 2;
    }
    return 0;
}

void disk_free(uint16_t address) {
    if (address == 0) return;
    uint16_t block_address = address - 2;
    uint16_t block_header = eeprom_read_word(block_address);
    if ((block_header & 0x8000) == 0) return;
    uint16_t block_size = block_header & 0x7fff;

    if (block_address - 2 >= DISK_BLOCK_ALIGN + 2) {
        uint16_t previous_block_header = eeprom_read_word(block_address - 2);
        if ((previous_block_header & 0x8000) == 0) {
            uint16_t previous_block_size = previous_block_header & 0x7fff;
            block_address -= 2 + previous_block_size + 2;
            block_size += 2 + previous_block_size + 2;
        }
    }

    if (block_address + 2 + block_size + 2 <= EEPROM_SIZE - 2 - 2) {
        uint16_t next_block_header = eeprom_read_word(block_address + 2 + block_size + 2);
        if ((next_block_header & 0x8000) == 0) {
            uint16_t next_block_size = next_block_header & 0x7fff;
            block_size += 2 + next_block_size + 2;
        }
    }

    eeprom_write_word(block_address, block_size);
    eeprom_write_word(block_address + 2 + block_size, block_size);
}

void disk_format(void) {
    #ifdef DEBUG
        for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
            eeprom_write_word(i, 0);
        }
    #endif

    eeprom_write_byte(DISK_HEADER_SIGNATURE, 'G');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 1, 'O');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 2, 'L');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 3, 'D');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 4, 'F');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 5, 'S');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 6, '\0');
    eeprom_write_byte(DISK_HEADER_VERSION, (VERSION_MAJOR << 4) | VERSION_MINOR);

    uint16_t free_block_size = EEPROM_SIZE - DISK_HEADER_SIZE - 2 - 2;
    eeprom_write_word(DISK_HEADER_SIZE, free_block_size);
    eeprom_write_word(EEPROM_SIZE - 2, free_block_size);
}

void disk_inspect(void) {
    serial_println_P(PSTR("Disk blocks:"));

    uint16_t block_address = DISK_BLOCK_ALIGN;
    uint16_t free_block_count = 0;
    uint16_t free_blocks_size = 0;
    uint16_t max_free_block_size = 0;
    while (EEPROM_SIZE - block_address >= 2 + 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;

        serial_print_P(PSTR("- "));
        serial_print_word(block_address + 2, '0');

        if ((block_header & 0x8000) == 0) {
            serial_print_P(PSTR(": Free block of "));
            serial_print_number(block_size, '\0');
            serial_println_P(PSTR(" bytes"));

            free_block_count++;
            free_blocks_size += block_size;
            if (block_size > max_free_block_size) {
                max_free_block_size = block_size;
            }
        } else {
            serial_print_P(PSTR(": Allocated block of "));
            serial_print_number(block_size, '\0');
            serial_println_P(PSTR(" bytes"));
        }

        block_address += 2 + block_size + 2;
    }

    serial_print_P(PSTR("\nFree blocks size is "));
    serial_print_number(free_blocks_size, '\0');
    serial_print_P(PSTR(" bytes\nSeperated over "));
    serial_print_number(free_block_count, '\0');
    serial_print_P(PSTR(" free blocks\nLargest free block is "));
    serial_print_number(max_free_block_size, '\0');
    serial_println_P(PSTR(" bytes"));
}
