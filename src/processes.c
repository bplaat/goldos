#include "processes.h"
#include "file.h"
#include "serial.h"

Process processes[PROCESSES_SIZE] = {0};

int8_t process_open(char *name, bool debug) {
    for (uint8_t i = 0; i < PROCESSES_SIZE; i++) {
        if (processes[i].niceness == 0) {
            int8_t file = file_open(name, FILE_OPEN_MODE_READ);
            if (file != -1) {
                processes[i].niceness = 1;
                processes[i].file = file;
                processes[i].state = PROCESS_STATE_RUNNING;
                processor_init(&processes[i].processor, debug, files[file].address + 1 + files[file].name_size + 2); // DIRTY
                return i;
            } else {
                return -1;
            }
        }
    }
    return -1;
}

bool process_sleep(int8_t process) {
    if (process >= 0 && process < PROCESSES_SIZE && processes[process].niceness != 0) {
        processes[process].state = PROCESS_STATE_SLEEPING;
        return true;
    }
    return false;
}

bool process_wake(int8_t process) {
    if (process >= 0 && process < PROCESSES_SIZE && processes[process].niceness != 0) {
        processes[process].state = PROCESS_STATE_RUNNING;
        return true;
    }
    return false;
}

bool process_niceness(int8_t process, uint8_t niceness) {
    if (niceness == 0) niceness = 1;
    if (niceness > 10) niceness = 10;

    if (process >= 0 && process < PROCESSES_SIZE && processes[process].niceness != 0) {
        processes[process].niceness = niceness;
        return true;
    }
    return false;
}

bool process_wait(int8_t process) {
    if (process >= 0 && process < PROCESSES_SIZE && processes[process].niceness != 0) {
        bool runToClose = false;
        while (processes[process].processor.running) {
            processor_clock(&processes[process].processor);

            if (!runToClose && processes[process].processor.debug) {
                while (serial_available() == 0);
                char character = serial_read();

                if (character == 's') {
                    while (
                        processor_clock(&processes[process].processor) != PROCESSOR_STATE_CALL &&
                        processes[process].processor.running
                    );
                }
                if (character == 'e') {
                    while (
                        processor_clock(&processes[process].processor) != PROCESSOR_STATE_RETURN &&
                        processes[process].processor.running
                    );
                }
                if (character == 'r') {
                    runToClose = true;
                }
                if (character == 'q') {
                    break;
                }
            }
        }
        process_close(process);
    }
    return false;
}

bool process_close(int8_t process) {
    if (process >= 0 && process < PROCESSES_SIZE && processes[process].niceness != 0) {
        processes[process].niceness = 0;
        file_close(processes[process].file);
        return true;
    }
    return false;
}

void processes_run(void) {
    for (uint8_t i = 0; i < PROCESSES_SIZE; i++) {
        if (processes[i].niceness != 0 && processes[i].state == PROCESS_STATE_RUNNING) {
            for (int16_t j = 0; j < processes[i].niceness << 4; j++) {
                processor_clock(&processes[i].processor);
                if (!processes[i].processor.running) {
                    process_close(i);
                    continue;
                }
            }
        }
    }
}
