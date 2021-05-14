#include "serial.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>

uint8_t serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];
uint8_t serial_input_read_position = 0;
uint8_t serial_input_write_position = 0;

ISR(USART_RX_vect) {
    uint8_t character = UDR0;
    if (serial_input_write_position == SERIAL_INPUT_BUFFER_SIZE) {
        serial_input_write_position = 0;
    }
    serial_input_buffer[serial_input_write_position++] = character;
}

void serial_begin(void) {
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
}

uint8_t serial_available(void) {
    return serial_input_write_position - serial_input_read_position;
}

uint8_t serial_read(void) {
    if (serial_input_write_position != serial_input_read_position) {
        if (serial_input_read_position == SERIAL_INPUT_BUFFER_SIZE) {
            serial_input_read_position = 0;
        }
        return serial_input_buffer[serial_input_read_position++];
    }
    return '\0';
}

void serial_write(uint8_t character) {
    if (character == '\n') {
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = '\r';
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = character;
}

void serial_print(uint8_t* string) {
    while (*string != '\0') {
        serial_write(*string);
        string++;
    }
}

void serial_print_progmem(const uint8_t* string) {
    uint8_t character;
    while ((character = pgm_read_byte(string)) != '\0') {
        serial_write(character);
        string++;
    }
}

void serial_println(uint8_t* string) {
    serial_print(string);
    serial_write('\n');
}

void serial_println_progmem(const uint8_t* string) {
    serial_print_progmem(string);
    serial_write('\n');
}
