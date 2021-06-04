rm -f -r .vscode goldos*

if [[ $1 == "arduino" ]]; then
    PATH=$PATH:"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin"
    if avr-gcc -Wall -Wextra -Wpedantic -Werror --std=c99 -Os \
        -DARDUINO -DDEBUG -DF_CPU=16000000UL -DBAUD=9600UL -DEEPROM_SIZE=1024 \
        -mmcu=atmega328p -Iinclude/ $(find src -name *.c) -o goldos
    then
        if [[ $2 == "disasm" ]]; then
            avr-size goldos
            avr-objdump -s -j .data goldos
            avr-objdump -S goldos > goldos.s
        else
            avr-objcopy -O ihex -R .eeprom goldos goldos.hex
            avrdude -C "C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" \
                -D -c arduino -v -p atmega328p -P COM9 -b 57600 -U flash:w:goldos.hex
            rm goldos.hex
            putty -serial COM9
        fi
    fi
else
    if gcc -Wall -Wextra -Wpedantic -Werror --std=c99 -Os -DDEBUG -DEEPROM_SIZE=256 \
        -Iinclude/ $(find src -name *.c) -o goldos
    then
        if [[ $1 == "disasm" ]]; then
            size goldos*
            objdump -M intel -S goldos* > goldos.s
        else
            ./goldos
        fi
    fi
fi
