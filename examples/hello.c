#include "goldos-dev.h"
#include <string.h>

void main(void) {
    serial_println(strrev("Hello World!"));
}
