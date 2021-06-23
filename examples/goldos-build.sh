PATH=$PATH:"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin"
if
    avr-gcc -Os -mmcu=attiny25 $1.c -o $1 \
        -Wl,--defsym,serial_write=2 -Wl,--defsym,serial_print=4 -Wl,--defsym,serial_print_P=6 \
        -Wl,--defsym,serial_println=8 -Wl,--defsym,serial_println_P=10 \
        -Wl,--defsym,file_open=12 -Wl,--defsym,file_name=14 -Wl,--defsym,file_size=16 \
        -Wl,--defsym,file_position=18 -Wl,--defsym,file_seek=20 -Wl,--defsym,file_read=22 \
        -Wl,--defsym,file_write=24 -Wl,--defsym,file_close=26
then
    if [[ $2 == "disasm" ]]; then
        avr-size $1
        avr-objdump -S $1 > $1.s
    fi
    avr-objcopy -O binary -R .eeprom $1 $1.prg
    rm $1
fi
