#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdint.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define WRITE_BUFFER_SIZE 16

uint8_t *file_read(char *path, size_t *file_size) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Can't read file: %s!\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *file_buffer = malloc(*file_size);
    fread(file_buffer, 1, *file_size, file);
    fclose(file);
    return file_buffer;
}

int main(int argc, char **argv) {
    printf("GoldOS File Uploader\n");

    if (argc >= 3) {
        size_t real_file_size;
        char *file_data = file_read(argv[2], &real_file_size);

        char serial_port_buffer[32];
        sprintf(serial_port_buffer, "\\\\.\\%s", argv[1]);
        HANDLE serial_port = CreateFileA(serial_port_buffer, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (serial_port == INVALID_HANDLE_VALUE) {
            printf("Can't open serial port: %s!\n", argv[1]);
            // return EXIT_FAILURE;
        }

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        dcbSerialParams.BaudRate = 9600;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        SetCommState(serial_port, &dcbSerialParams);
        COMMTIMEOUTS timeoutParams;
        timeoutParams.ReadIntervalTimeout = 2; // wait 2 ms for each character (9600 bps = 1.04 ms per character)
        SetCommTimeouts(serial_port, &timeoutParams);
        EscapeCommFunction(serial_port, SETDTR); // reset Arduino
        Sleep(1000);
        EscapeCommFunction(serial_port, CLRDTR);
        Sleep(1000);

        // Start command
        char buffer[32];
        sprintf(buffer, "write %s\r\n", argv[2]);
        DWORD bytes_written;
        WriteFile(serial_port, buffer, strlen(buffer), &bytes_written, NULL);
        printf("%s", buffer);

        // Send file
        uint16_t file_size = real_file_size;
        uint16_t rest = file_size;
        for (uint16_t i = 0; i < file_size; i += WRITE_BUFFER_SIZE) {
            for (uint8_t j = 0; j < MIN(rest, WRITE_BUFFER_SIZE); j++) {
                sprintf(buffer, "%02x", file_data[i + j]);
                WriteFile(serial_port, buffer, strlen(buffer), &bytes_written, NULL);
                printf("%s", buffer);
            }

            rest -= WRITE_BUFFER_SIZE;

            strcpy(buffer, "\r\n");
            WriteFile(serial_port, buffer, strlen(buffer), &bytes_written, NULL);
            printf("%s", buffer);
        }

        strcpy(buffer, "\r\n");
        WriteFile(serial_port, buffer, strlen(buffer), &bytes_written, NULL);
        printf("%s", buffer);

        CloseHandle(serial_port);

        free(file_data);
    } else {
        printf("Help: ./receive COM9 file.txt\n");
    }

    return EXIT_SUCCESS;
}
