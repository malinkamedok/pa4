#include "pipes.h"
#include "ipc.h"

timestamp_t read_lamport_time(pipes_all_global* pipesAllGlobal);

void init_lamport_time(pipes_all_global* pipesAllGlobal);

void incr_lamport_time(pipes_all_global* pipesAllGlobal);

void set_lamport_time(pipes_all_global* pipesAllGlobal, timestamp_t new_time);

