#include "directory.h"

int8_t directory_open(char *path, uint8_t mode) {
    (void)path;
    (void)mode;
    return 0;
}

bool directory_read(int8_t directory, uint8_t *type, char *name, uint16_t *size) {
    (void)directory;
    (void)type;
    (void)name;
    (void)size;
    return false;
}

bool directory_close(int8_t directory) {
    (void)directory;
    return false;
}
