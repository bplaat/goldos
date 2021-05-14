#include "serial.h"
#include "commands.h"
#include "utils.h"

#define INPUT_BUFFER_SIZE 64
uint8_t input_buffer[INPUT_BUFFER_SIZE];
bool input_buffer_ready = false;
uint8_t input_buffer_position = 0;

uint8_t directory[] = "/";
const uint8_t prompt[] PROGMEM = " $ ";

uint8_t argc;
uint8_t* argv[8];
void parse_arguments(char* arguments) {
    argc = 0;
    uint8_t* pointer = arguments;
    while (*pointer != '\0') {
        if (*pointer == '"') {
            pointer++;
            argv[argc++] = pointer;
            while (*pointer != '"') {
                if (*pointer == '\0') return;
                pointer++;
            }
            *pointer = '\0';
            pointer++;
        }
        else if (*pointer == '\'') {
            pointer++;
            argv[argc++] = pointer;
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
            argv[argc++] = pointer;
            while (*pointer != ' ') {
                if (*pointer == '\0') return;
                pointer++;
            }
            *pointer = '\0';
            pointer++;
        }
    }
}

void main(void) {
    serial_begin();
    serial_println_progmem(PSTR("\e[2J\e[;HGoldOS for the ATmega328p microcontroller!"));
    serial_print(directory);
    serial_print_progmem(prompt);

    for (;;) {
        if (serial_available() > 0) {
            uint8_t character;
            while ((character = serial_read()) != '\0') {
                // Delete / Backspace character
                if (character == 0x7f && input_buffer_position > 0) {
                    input_buffer[input_buffer_position--] = '\0';
                    serial_write(0x7f); // Print delete character
                }

                // Enter character CRLF
                if (character == '\r') {
                    serial_read(); // Skip next \n character
                    input_buffer[input_buffer_position] = '\0';
                    serial_write('\n'); // Print enter character
                    input_buffer_ready = true;
                }

                // Normal ASCII characters
                if (character >= ' ' && character <= '~') {
                    input_buffer[input_buffer_position++] = character;
                    input_buffer[input_buffer_position] = '\0';
                    serial_write(character);
                }

                // Ignore the rest
            }
        }

        if (input_buffer_ready) {
            // Trim input buffer
            uint8_t* input_buffer_trimmed = string_trim(input_buffer);

            // When command is given try to run it
            if (input_buffer_trimmed[0] != '\0') {
                // Parse arguments
                parse_arguments(input_buffer_trimmed);

                #ifdef DEBUG
                    serial_println_progmem(PSTR("Arguments:"));
                    for (uint8_t i = 0; i < argc; i++) {
                        serial_print_progmem(PSTR("- "));
                        serial_println(argv[i]);
                    }
                    serial_write('\n');
                #endif

                // Run the right command
                bool is_command_found = false;
                for (uint8_t i = 0; i < COMMANDS_SIZE; i++) {
                    Command command = commands[i];
                    if (string_equals(command.name, argv[0])) {
                        is_command_found = true;
                        command.command_function(argc, argv);
                        break;
                    }
                }

                // Print text when command is not found
                if (!is_command_found) {
                    serial_print_progmem(PSTR("Can't find command: "));
                    serial_println(argv[0]);
                }
            }

            // Print new prompt
            serial_print(directory);
            serial_print_progmem(prompt);

            // Clear input buffer
            input_buffer_position = 0;
            input_buffer_ready = false;
        }
    }
}
