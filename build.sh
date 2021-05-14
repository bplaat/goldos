PATH=$PATH:"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin"
if avr-gcc -Os -DF_CPU=16000000UL -DBAUD=9600UL -mmcu=atmega328p -Iinclude/ src/*.c -o goldos; then
    avr-size goldos

    if [[ $1 == "hex" ]]; then
        avr-objdump -S goldos > goldos.s
    fi

    if [[ $1 == "send" ]]; then
        avr-objcopy -O ihex -R .eeprom goldos goldos.hex
        avrdude -C "C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" \
            -D -c arduino -p atmega328p -P COM3 -b 115200 -U flash:w:goldos.hex
        rm goldos.hex
    fi

    rm goldos
fi
