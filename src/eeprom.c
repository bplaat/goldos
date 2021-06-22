#include "eeprom.h"
#ifdef ARDUINO
    #include <avr/io.h>
    #include <avr/interrupt.h>
#else
    #include <stdio.h>
#endif
#include "serial.h"

#ifndef ARDUINO
    uint8_t eeprom_data[EEPROM_SIZE] = {0};

    void eeprom_begin(void) {
        FILE *eeprom_file = fopen("eeprom.bin", "rb");
        if (eeprom_file != NULL) {
            fread(&eeprom_data, 1, EEPROM_SIZE, eeprom_file);
            fclose(eeprom_file);
        }
    }

    void eeprom_end(void) {
        FILE *eeprom_file = fopen("eeprom.bin", "wb");
        fwrite(&eeprom_data, 1, EEPROM_SIZE, eeprom_file);
        fclose(eeprom_file);
    }
#endif

uint8_t eeprom_read_byte(uint16_t address) {
    #ifdef ARDUINO
        loop_until_bit_is_clear(EECR, EEPE);
        EEAR = address;
        EECR |= _BV(EERE);
        return EEDR;
    #else
        return eeprom_data[address];
    #endif
}

void eeprom_write_byte(uint16_t address, uint8_t byte) {
    if (eeprom_read_byte(address) != byte) {
        #ifdef ARDUINO
            loop_until_bit_is_clear(EECR, EEPE);
            EEAR = address;
            EEDR = byte;
            cli();
            EECR |= _BV(EEMPE);
            EECR |= _BV(EEPE);
            sei();
        #else
            eeprom_data[address] = byte;
        #endif
    }
}

uint16_t eeprom_read_word(uint16_t address) {
    return eeprom_read_byte(address) | (eeprom_read_byte(address + 1) << 8);
}

void eeprom_write_word(uint16_t address, uint16_t word) {
    eeprom_write_byte(address, word & 0xff);
    eeprom_write_byte(address + 1, word >> 8);
}

void eeprom_dump(void) {
    serial_print_P(PSTR("     "));
    for (uint8_t x = 0; x < 16; x++) {
        serial_print_byte(x, ' ');
        serial_write(x == 15 ? '\t' : ' ');
    }
    for (uint8_t x = 0; x < 16; x++) {
        serial_print_byte(x, '\0');
        serial_write(x == 15 ? '\n' : ' ');
    }

    for (uint16_t y = 0; y < (EEPROM_SIZE >> 4); y++) {
        serial_print_word(y << 4, '0');
        serial_write(' ');

        for (size_t x = 0; x < 16; x++) {
            serial_print_byte(eeprom_read_byte((y << 4) + x), '0');
            serial_write(x == 15 ? '\t' : ' ');
        }

        for (size_t x = 0; x < 16; x++) {
            char character = eeprom_read_byte((y << 4) + x);
            if (character < ' ' || character > '~') {
                character = '.';
            }
            serial_write(character);
            serial_write(x == 15 ? '\n' : ' ');
        }
    }
}
