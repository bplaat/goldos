# The GoldOS build script

ARDUINO_PORT="COM3"

rm goldos*

if [[ $1 == "arduino" ]]; then
    # Build and upload GoldOS for your Arduino Uno
    PATH=$PATH:"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin"
    if avr-gcc -Wall -Wextra -Wpedantic --std=c11 -Os -DARDUINO -DDEBUG -DF_CPU=16000000UL -DBAUD=9600UL \
        -mmcu=atmega328p -Iinclude/ src/*.c -o goldos
    then
        if [[ $2 == "disasm" ]]; then
            avr-size goldos
            avr-objdump -S goldos > goldos.s
        else
            avr-objcopy -O ihex -R .eeprom goldos goldos.hex
            avrdude -C "C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" \
                -D -c arduino -p atmega328p -P $ARDUINO_PORT -b 115200 -U flash:w:goldos.hex
            rm goldos.hex
        fi
    fi
else
    # Build GoldOS for your computer
    if gcc -Wall -Wextra -Wpedantic --std=c11 -Werror -Os -DDEBUG -Iinclude/ src/*.c -o goldos; then
        if [[ $1 == "disasm" ]]; then
            size goldos*
            objdump -M intel -S goldos* > goldos.s
        else
            ./goldos
        fi
    fi
fi
