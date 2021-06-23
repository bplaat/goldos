#include "goldos-dev.h"

void main(void) {
    int8_t file = file_open("test.txt", FILE_OPEN_MODE_APPEND);
    if (file != -1) {
        char *string = "HEJA!\n";
        file_write(file, string, -1);
        file_close(file);
    }
}
