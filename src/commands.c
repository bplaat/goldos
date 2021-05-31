#include "commands.h"
#ifdef ARDUINO
    #include <avr/wdt.h>
#endif
#include <stdlib.h>
#include "serial.h"
#include "eeprom.h"

const PROGMEM char sum_command_name[] = "sum";
const PROGMEM char average_command_name[] = "average";
const PROGMEM char hello_command_name[] = "hello";
const PROGMEM char help_command_name[] = "help";
const PROGMEM char exit_command_name[] = "exit";
const PROGMEM char read_command_name[] = "read";
const PROGMEM char write_command_name[] = "write";
const PROGMEM char clear_command_name[] = "clear";
const PROGMEM char pause_command_name[] = "pause";

const Command commands[] PROGMEM = {
    { sum_command_name, &sum_command },
    { average_command_name, &average_command },
    { hello_command_name, &hello_command },
    { help_command_name, &help_command },
    { exit_command_name, &exit_command },
    { read_command_name, &read_command },
    { write_command_name, &write_command },
    { clear_command_name, &clear_command },
    { pause_command_name, &pause_command }
};

void sum_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += atoi(argv[i]);
        }

        char number_buffer[7];
        itoa(sum, number_buffer, 10);
        serial_println(number_buffer);
    } else {
        serial_println_P(PSTR("Help: sum [number]..."));
    }
}

void average_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += atoi(argv[i]);
        }

        char number_buffer[7];
        itoa(sum / (argc - 1), number_buffer, 10);
        serial_println(number_buffer);
    } else {
        serial_println_P(PSTR("Help: average [number]..."));
    }
}

void hello_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        serial_print_P(PSTR("Hello "));
        for (uint8_t i = 1; i < argc; i++) {
            serial_print(argv[i]);
            if (i != argc - 1) {
                serial_write(' ');
            }
        }
        serial_println_P(PSTR("!"));
    } else {
        serial_println_P(PSTR("Hello World!"));
    }
}

void help_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    serial_println_P(PSTR("Commands:"));
    for (uint8_t i = 0; i < COMMANDS_SIZE; i++) {
        serial_print_P(PSTR("- "));
        serial_println_P((const char *)pgm_read_word(&commands[i].name));
    }
}

void exit_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    #ifdef ARDUINO
        wdt_enable(WDTO_15MS);
        for (;;);
    #else
        exit(0);
    #endif
}

void read_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    uint16_t position = 0;
    char character;
    while ((character = eeprom_read(position++)) != '\0') {
        serial_write(character);
    }
}

void write_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        uint16_t position = 0;
        for (uint8_t i = 1; i < argc; i++) {
            char *argument = argv[i];
            while (*argument != '\0') {
                eeprom_write(position++, *argument++);
            }
            eeprom_write(position++, ' ');
        }
        eeprom_write(position++, '\n');
        eeprom_write(position++, '\0');
    } else {
        serial_println_P(PSTR("Help: write [text]..."));
    }
}

void clear_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    serial_print_P(PSTR("\x1b[2J\x1b[;H"));
}

void pause_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    serial_print_P(PSTR("Press any key to continue..."));
    while (serial_available() == 0);
    serial_read();
    serial_write('\n');
}
