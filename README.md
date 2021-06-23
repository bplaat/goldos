# bplaat/goldos
A simple Operating System for AVR microcontrollers and runnable as a normal program

## TODO
- Proccessor foreign functions

## How to build it?
Run this command to build and run GoldOS on your own computer (Windows only for the moment):
```
./build.sh
```

Run this command to build and upload GoldOS to your Arduino:
```
./build.sh arduino
```
You will need to install the Arduino IDE and set the serial port for this to work

## Tools
In the `tools/` folder are some tools that you can use to do stuff (these are also Windows only for the moment):
- `send.c` Send files to your Arduino running GoldOS
