#include "lamport.h"
#include "pipes.h"
#include "ipc.h"

timestamp_t read_lamport_time(pipes_all_global* pipesAllGlobal) {
    return pipesAllGlobal->lamport_time;
}

void init_lamport_time(pipes_all_global* pipesAllGlobal) {
    pipesAllGlobal->lamport_time = 0;
}

void incr_lamport_time(pipes_all_global* pipesAllGlobal) {
    pipesAllGlobal->lamport_time = pipesAllGlobal->lamport_time + 1;
}

void set_lamport_time(pipes_all_global* pipesAllGlobal, timestamp_t new_time) {
    pipesAllGlobal->lamport_time = new_time;
}
