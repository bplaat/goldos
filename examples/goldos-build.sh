PATH=$PATH:"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin"
if avr-gcc -Os -mmcu=attiny25 $1.c -o $1; then
    if [[ $2 == "disasm" ]]; then
        avr-size $1
        avr-objdump -S $1 > $1.s
    fi
    avr-objcopy -O binary -R .eeprom $1 $1.prg
    rm $1
fi
