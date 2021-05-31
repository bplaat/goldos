#ifndef EEPROM_H

#include <stdint.h>

#ifndef ARDUINO
    void eeprom_begin(void);
#endif

uint8_t eeprom_read(uint16_t address);

void eeprom_write(uint16_t address, uint8_t data);

#endif
