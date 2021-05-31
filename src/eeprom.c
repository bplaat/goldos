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
#endif

uint8_t eeprom_read(uint16_t address) {
    #ifdef ARDUINO
        loop_until_bit_is_clear(EECR, EEPE);
        EEAR = address;
        EECR |= _BV(EERE);
        return EEDR;
    #else
        return eeprom_data[address];
    #endif
}

void eeprom_write(uint16_t address, uint8_t data) {
    if (eeprom_read(address) != data) {
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
            fseek(eeprom_file, 0, SEEK_SET);
            fwrite(&eeprom_data, 1, EEPROM_SIZE, eeprom_file);
        #endif
    }
}
