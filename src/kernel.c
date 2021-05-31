#include <string.h>
#include <stdbool.h>
#include "serial.h"
#include "eeprom.h"
#include "commands.h"

#define INPUT_BUFFER_SIZE 48

char input_buffer[INPUT_BUFFER_SIZE];

uint8_t input_buffer_size;

#define ARGUMENTS_MAX 8

const PROGMEM char prompt[] = "> ";

void arguments_parse(char *buffer, char **arguments, uint8_t *size, uint8_t max_size) {
    *size = 0;

    char *pointer = buffer;
    while (*pointer != '\0') {
        if (*size == max_size) return;

        if (*pointer == '"') {
            pointer++;
            arguments[(*size)++] = pointer;
            while (*pointer != '"') {
                if (*pointer == '\0') return;
                pointer++;
            }
            *pointer = '\0';
            pointer++;
        }

        else if (*pointer == '\'') {
            pointer++;
            arguments[(*size)++] = pointer;
            while (*pointer != '\'') {
                if (*pointer == '\0') return;
                pointer++;
            }
            *pointer = '\0';
            pointer++;
        }

        else if (*pointer == ' ') {
            pointer++;
        }

        else {
            arguments[(*size)++] = pointer;
            while (*pointer != ' ') {
                if (*pointer == '\0') return;
                pointer++;
            }
            *pointer = '\0';
            pointer++;
        }
    }
}

int main(void) {
    serial_begin();

    #ifndef ARDUINO
        eeprom_begin();
    #endif

    serial_println_P(PSTR("\x1b[2J\x1b[;H\x1b[32mGoldOS v0.1\x1b[0m"));

    for (;;) {
        serial_print_P(prompt);
        serial_read_line(input_buffer, &input_buffer_size, INPUT_BUFFER_SIZE);

        if (input_buffer[0] != '\0') {
            char *arguments[ARGUMENTS_MAX];
            uint8_t arguments_size;
            arguments_parse(input_buffer, arguments, &arguments_size, ARGUMENTS_MAX);

            #ifdef DEBUG
                serial_println_P(PSTR("Arguments:"));
                for (uint8_t i = 0; i < arguments_size; i++) {
                    serial_print_P(PSTR("- "));
                    serial_println(arguments[i]);
                }
                serial_write('\n');
            #endif

            bool is_command_found = false;
            for (uint8_t i = 0; i < COMMANDS_SIZE; i++) {
                Command command = commands[i];
                if (!strcmp_P(arguments[0], command.name)) {
                    is_command_found = true;
                    command.command_function(arguments_size, arguments);
                    break;
                }
            }
            if (!is_command_found) {
                serial_print_P(PSTR("Can't find command: "));
                serial_println(arguments[0]);
            }
        }
    }

    return 0;
}
