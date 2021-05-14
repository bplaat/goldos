#include "commands.h"
#include "serial.h"
#include "utils.h"

void sum_command(uint8_t argc, uint8_t** argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += string_to_int(argv[i]);
        }

        uint8_t number_buffer[7];
        int_to_string(number_buffer, sum, 10);
        serial_println(number_buffer);
    } else {
        serial_println_progmem(PSTR("Help: sum [number]..."));
    }
}

void average_command(uint8_t argc, uint8_t** argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += string_to_int(argv[i]);
        }

        uint8_t number_buffer[7];
        int_to_string(number_buffer, sum / (argc - 1), 10);
        serial_println(number_buffer);
    } else {
        serial_println_progmem(PSTR("Help: average [number]..."));
    }
}

void hello_command(uint8_t argc, uint8_t** argv) {
    if (argc >= 2) {
        serial_print_progmem(PSTR("Hello "));
        for (uint8_t i = 1; i < argc; i++) {
            serial_print(argv[i]);
            if (i != argc - 1) {
                serial_write(' ');
            }
        }
        serial_println_progmem(PSTR("!"));
    } else {
        serial_println_progmem(PSTR("Hello World!"));
    }
}

void help_command(uint8_t argc, uint8_t** argv) {
    serial_println_progmem(PSTR("Commands:"));
    for (uint8_t i = 0; i < COMMANDS_SIZE; i++) {
        Command command = commands[i];
        serial_print_progmem(PSTR("- "));
        serial_println(command.name);
    }
}

Command commands[] = {
    { "sum", &sum_command },
    { "average", &average_command },
    { "hello", &hello_command },
    { "help", &help_command }
};

const uint8_t COMMANDS_SIZE = sizeof(commands) / sizeof(Command);
