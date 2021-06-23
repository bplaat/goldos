#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "processor.h"

typedef enum ProcessState {
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_SLEEPING
} ProcessState;

typedef struct Process {
    uint8_t niceness;
    int8_t file;
    ProcessState state;
    Processor processor;
} Process;

#define PROCESSES_SIZE 3

extern Process processes[PROCESSES_SIZE];

int8_t process_open(char *name, bool debug);

bool process_sleep(int8_t process);

bool process_wake(int8_t process);

bool process_niceness(int8_t process, uint8_t niceness);

bool process_wait(int8_t process);

bool process_close(int8_t process);

void processes_run(void);

#endif
