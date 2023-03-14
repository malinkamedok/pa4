#ifndef PA1_PIPES_H
#define PA1_PIPES_H

#include "mm_malloc.h"
#include "ipc.h"

struct pipes {
    int fd[2];
};

typedef struct {
    timestamp_t request_time;
    size_t id;
} request_queue;

request_queue * new_member_in_queue(timestamp_t time, size_t id);

void push (void * self, timestamp_t time, size_t id);

request_queue * pop (void * self);

int compare_elements (request_queue * element0, request_queue * element1);

void sort_elements (void * self);

typedef struct {
    size_t number_of_child_procs;
    struct pipes ** pipes_all;
    timestamp_t lamport_time;
    request_queue * queue[10];
    size_t queue_size;
    size_t done_procs;
} pipes_all_global;

pipes_all_global * new(size_t number);

typedef struct {
    size_t id;
    pipes_all_global * global_elite;
} baby_maybe_process;

baby_maybe_process * new_baby(size_t id, pipes_all_global * x);

#endif //PA1_PIPES_H
