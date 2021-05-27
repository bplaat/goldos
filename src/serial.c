#include "serial.h"
#ifdef ARDUINO
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <util/setbaud.h>
#else
    #ifdef __WIN32__
        #include <windows.h>
    #else
        #include <unistd.h>
        #include <fcntl.h>
        #include <stdio.h>
    #endif
#endif

char serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];

uint8_t serial_input_read_position = 0;

uint8_t serial_input_write_position = 0;

#ifdef ARDUINO
    ISR(USART_RX_vect) {
        char character = UDR0;
        if (serial_input_write_position == SERIAL_INPUT_BUFFER_SIZE) {
            serial_input_write_position = 0;
        }
        serial_input_buffer[serial_input_write_position++] = character;
    }
#endif

#ifdef __WIN32__
    HANDLE stdinHandle;
    HANDLE stdoutHandle;
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
        // Enable ANSI escape codes on Windows
        #ifdef __WIN32__
            stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
            DWORD consoleMode;
            GetConsoleMode(stdinHandle, &consoleMode);
            consoleMode &= ~ENABLE_LINE_INPUT;
            SetConsoleMode(stdinHandle, consoleMode);

            stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            GetConsoleMode(stdoutHandle, &consoleMode);
            consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(stdoutHandle, consoleMode);
        #else
            fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
        #endif
    #endif
}

#ifndef ARDUINO
    void serial_read_input(void) {
        char character;
        #if __WIN32__
            DWORD bytesRead;
            ReadConsole(stdinHandle, &character, 1, &bytesRead, NULL);
            if (bytesRead > 0) {
        #else
            if (read(STDIN_FILENO, &character, 1) > 0) {
        #endif
            if (serial_input_write_position == SERIAL_INPUT_BUFFER_SIZE) {
                serial_input_write_position = 0;
            }
            serial_input_buffer[serial_input_write_position++] = character;
        }
    }
#endif

uint8_t serial_available(void) {
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

void serial_write(char character) {
    if (character == '\n') {
        serial_write('\r');
    }

    #ifdef ARDUINO
        UDR0 = character;
        loop_until_bit_is_set(UCSR0A, UDRE0);
    #else
        #ifdef __WIN32__
            WriteConsole(stdoutHandle, &character, 1, NULL, NULL);
        #else
            putchar(character);
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
    void serial_print_progmem(const char *string) {
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
    void serial_println_progmem(const char *string) {
        serial_print_progmem(string);
        serial_write('\n');
    }
#endif
