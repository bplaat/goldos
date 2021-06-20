#include "serial.h"
#ifdef ARDUINO
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <util/setbaud.h>
#else
    #ifdef __WIN32__
        #include <windows.h>
    #endif
#endif
#include <stdlib.h>
#include <string.h>

char serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];

uint8_t serial_input_read_position = 0;

uint8_t serial_input_write_position = 0;

#ifdef __WIN32__
    HANDLE stdin_handle;

    HANDLE stdout_handle;
#endif

void serial_begin(void) {
    #ifdef ARDUINO
        UBRR0H = UBRRH_VALUE;
        UBRR0L = UBRRL_VALUE;

        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif

        UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
        UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

        sei();
    #else
        #ifdef __WIN32__
            stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
            DWORD console_mode;
            GetConsoleMode(stdin_handle, &console_mode);
            console_mode &= ~ENABLE_LINE_INPUT;
            SetConsoleMode(stdin_handle, console_mode);

            stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            GetConsoleMode(stdout_handle, &console_mode);
            console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(stdout_handle, console_mode);
        #endif
    #endif
}

#ifdef ARDUINO
    ISR(USART_RX_vect) {
        char character = UDR0;
        if (serial_input_write_position == SERIAL_INPUT_BUFFER_SIZE) {
            serial_input_write_position = 0;
        }
        serial_input_buffer[serial_input_write_position++] = character;
    }
#else
    void serial_read_input(void) {
        #if __WIN32__
            char character;
            DWORD bytes_read;
            ReadConsole(stdin_handle, &character, 1, &bytes_read, NULL);
            if (bytes_read > 0) {
                if (serial_input_write_position == SERIAL_INPUT_BUFFER_SIZE) {
                    serial_input_write_position = 0;
                }
                serial_input_buffer[serial_input_write_position++] = character;
            }
        #endif
    }
#endif

uint8_t serial_available(void) {
    #ifndef ARDUINO
        serial_read_input();
    #endif

    return serial_input_write_position - serial_input_read_position;
}

char serial_read(void) {
    if (serial_input_write_position != serial_input_read_position) {
        if (serial_input_read_position == SERIAL_INPUT_BUFFER_SIZE) {
            serial_input_read_position = 0;
        }
        return serial_input_buffer[serial_input_read_position++];
    }
    return '\0';
}

void serial_read_line(char *buffer, uint8_t *size, uint8_t max_size) {
    *size = 0;
    for (;;) {
        #ifndef ARDUINO
            serial_read_input();
        #endif

        char character;
        while ((character = serial_read()) != '\0') {
            if ((character >= ' ' && character <= '~') && *size < max_size - 1) {
                buffer[(*size)++] = character;
                serial_write(character);
            }

            if ((character == 8 || character == 127) && *size > 0) {
                buffer[(*size)--] = '\0';
                serial_write(8);
                serial_write(' ');
                serial_write(8);
            }

            if (character == '\r' || character == '\n') {
                if (character == '\r') {
                    serial_read();
                }

                buffer[*size] = '\0';
                serial_write('\n');
                return;
            }
        }
    }
}

void serial_write(char character) {
    if (character == '\n') {
        serial_write('\r');
    }

    #ifdef ARDUINO
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = character;
    #else
        #ifdef __WIN32__
            WriteConsole(stdout_handle, &character, 1, NULL, NULL);
        #endif
    #endif
}

void serial_print(char *string) {
    while (*string != '\0') {
        serial_write(*string);
        string++;
    }
}

#ifdef ARDUINO
    void serial_print_P(const char *string) {
        char character;
        while ((character = pgm_read_byte(string)) != '\0') {
            serial_write(character);
            string++;
        }
    }
#endif

void serial_println(char *string) {
    serial_print(string);
    serial_write('\n');
}

#ifdef ARDUINO
    void serial_println_P(const char *string) {
        serial_print_P(string);
        serial_write('\n');
    }
#endif

void serial_print_number(int16_t number, char padding) {
    char number_buffer[7];
    itoa(number, number_buffer, 10);
    if (padding != '\0') {
        uint8_t diff = (sizeof(number_buffer) - 1) - strlen(number_buffer);
        for (uint8_t i = 0; i < diff; i++) serial_write(padding);
    }
    serial_print(number_buffer);
}

void serial_print_byte(uint8_t byte, char padding) {
    char number_buffer[3];
    itoa(byte, number_buffer, 16);
    if (padding != '\0') {
        uint8_t diff = (sizeof(number_buffer) - 1) - strlen(number_buffer);
        for (uint8_t i = 0; i < diff; i++) serial_write(padding);
    }
    serial_print(number_buffer);
}

void serial_print_word(uint16_t word, char padding) {
    char number_buffer[5];
    itoa(word, number_buffer, 16);
    if (padding != '\0') {
        uint8_t diff = (sizeof(number_buffer) - 1) - strlen(number_buffer);
        for (uint8_t i = 0; i < diff; i++) serial_write(padding);
    }
    serial_print(number_buffer);
}

void serial_print_memory(uint8_t *data, uint16_t size) {
    serial_print_P(PSTR("     "));

    for (uint8_t x = 0; x < 16; x++) {
        serial_print_byte(x, ' ');
        serial_write(x == 15 ? '\t' : ' ');
    }

    for (uint8_t x = 0; x < 16; x++) {
        serial_print_byte(x, '\0');
        serial_write(x == 15 ? '\n' : ' ');
    }

    for (uint16_t y = 0; y < (size >> 4); y++) {
        serial_print_word(y << 4, '0');
        serial_write(' ');

        for (size_t x = 0; x < 16; x++) {
            serial_print_byte(data[(y << 4) + x], '0');
            serial_write(x == 15 ? '\t' : ' ');
        }

        for (size_t x = 0; x < 16; x++) {
            char character = data[(y << 4) + x];
            if (character  < ' ' || character > '~') {
                character = '.';
            }
            serial_write(character);
            serial_write(x == 15 ? '\n' : ' ');
        }
    }
}
