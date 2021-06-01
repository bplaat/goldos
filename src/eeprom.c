#include "eeprom.h"
#ifdef ARDUINO
    #include <avr/io.h>
    #include <avr/interrupt.h>
#else
    #include <stdio.h>
#endif

#ifndef ARDUINO
    FILE *eeprom_file;

    uint8_t eeprom_data[EEPROM_SIZE] = {0};

    void eeprom_begin(void) {
        eeprom_file = fopen("eeprom.bin", "r+");
        if (eeprom_file != NULL) {
            fread(&eeprom_data, 1, EEPROM_SIZE, eeprom_file);
        } else {
            eeprom_file = fopen("eeprom.bin", "w");
            fwrite(&eeprom_data, 1, EEPROM_SIZE, eeprom_file);
        }
    }

    void eeprom_end(void) {
        fseek(eeprom_file, 0, SEEK_SET);
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

void eeprom_write_byte(uint16_t address, uint8_t data) {
    if (eeprom_read_byte(address) != data) {
        #ifdef ARDUINO
            loop_until_bit_is_clear(EECR, EEPE);
            EEAR = address;
            EEDR = data;
            cli();
            EECR |= _BV(EEMPE);
            EECR |= _BV(EEPE);
            sei();
        #else
            eeprom_data[address] = data;
        #endif
    }
}

uint16_t eeprom_read_word(uint16_t address) {
    return eeprom_read_byte(address) | (eeprom_read_byte(address + 1) << 8);
}

void eeprom_write_word(uint16_t address, uint16_t data) {
    eeprom_write_byte(address, data & 0xff);
    eeprom_write_byte(address + 1, data >> 8);
}
