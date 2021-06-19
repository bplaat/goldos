#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

#ifndef EEPROM_SIZE
    #define EEPROM_SIZE 1
#endif

#ifndef ARDUINO
    void eeprom_begin(void);

    void eeprom_end(void);
#endif

uint8_t eeprom_read_byte(uint16_t address);

void eeprom_write_byte(uint16_t address, uint8_t data);

uint16_t eeprom_read_word(uint16_t address);

void eeprom_write_word(uint16_t address, uint16_t data);

void eeprom_dump(void);

#endif
