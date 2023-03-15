#include "pa1.h"
#include "pipes.h"
#include "lamport.h"
#include <stdio.h>
#include <unistd.h>


void swap(request_queue *xp, request_queue *yp)
{
    request_queue temp = *xp;
    *xp = *yp;
    *yp = temp;
}
// A function to implement bubble sort
void bubbleSort(void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    int n = babyMaybeProcess->global_elite->queue_size;
    int i, j;
    for (i = 0; i < n-1; i++)
// Last i elements are already in place
        for (j = 0; j < n-i-1; j++)
            if (babyMaybeProcess->global_elite->queue[j]->request_time > babyMaybeProcess->global_elite->queue[j+1]->request_time) {
                swap(babyMaybeProcess->global_elite->queue[j], babyMaybeProcess->global_elite->queue[j+1]);
            } else if (babyMaybeProcess->global_elite->queue[j]->request_time == babyMaybeProcess->global_elite->queue[j+1]->request_time && babyMaybeProcess->global_elite->queue[j]->id > babyMaybeProcess->global_elite->queue[j+1]->id) {
                swap(babyMaybeProcess->global_elite->queue[j], babyMaybeProcess->global_elite->queue[j+1]);
            }
}

void sort_elements (void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    bubbleSort(babyMaybeProcess);

//    for (size_t i = 0; i < babyMaybeProcess->global_elite->queue_size; i++) {
//        printf("%zu sorted array: %d %zu\n", babyMaybeProcess->id, babyMaybeProcess->global_elite->queue[i]->request_time, babyMaybeProcess->global_elite->queue[i]->id);
//    }
}

void push (void * self, timestamp_t time, size_t id) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    request_queue * requestQueue = new_member_in_queue(time, id);
    babyMaybeProcess->global_elite->queue[babyMaybeProcess->global_elite->queue_size] = requestQueue;

    babyMaybeProcess->global_elite->queue_size = babyMaybeProcess->global_elite->queue_size + 1;

    sort_elements(babyMaybeProcess);

//    printf("%zu Sorted arr after push\n", babyMaybeProcess->id);
//    for (size_t i = 0; i < babyMaybeProcess->global_elite->queue_size; i++) {
//        printf("%zu i: %zu time: %d id: %zu\n", babyMaybeProcess->id, i, babyMaybeProcess->global_elite->queue[i]->request_time, babyMaybeProcess->global_elite->queue[i]->id);
//    }

    return;
}

void pop (void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    for (size_t i = 0; i < babyMaybeProcess->global_elite->queue_size - 1; i++) {
        babyMaybeProcess->global_elite->queue[i] = babyMaybeProcess->global_elite->queue[i+1];
    }

    babyMaybeProcess->global_elite->queue_size = babyMaybeProcess->global_elite->queue_size - 1;

    return;
}

int request_cs(const void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    incr_lamport_time(babyMaybeProcess->global_elite);

    Message request_message;
    MessageHeader messageHeader;
    messageHeader.s_type = CS_REQUEST;
    messageHeader.s_magic = MESSAGE_MAGIC;
    messageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
    messageHeader.s_payload_len = 0;
    request_message.s_header = messageHeader;

    for (size_t i = 1; i < babyMaybeProcess->global_elite->number_of_child_procs; i++) {
        if (i != babyMaybeProcess->id) {
            send(babyMaybeProcess, i, &request_message);
        }
    }

    push(babyMaybeProcess, babyMaybeProcess->global_elite->lamport_time, babyMaybeProcess->id);

//    printf("first push in request time: %d id: %zu\n", babyMaybeProcess->global_elite->queue[0]->request_time, babyMaybeProcess->global_elite->queue[0]->id);

    Message any_message_receive;
    int replays_got = 0;
    while (1) {
        while (receive_any(babyMaybeProcess, &any_message_receive) == -1) {}

        if (read_lamport_time(babyMaybeProcess->global_elite) < any_message_receive.s_header.s_local_time) {
            set_lamport_time(babyMaybeProcess->global_elite, any_message_receive.s_header.s_local_time);
            incr_lamport_time(babyMaybeProcess->global_elite);
        } else {
            incr_lamport_time(babyMaybeProcess->global_elite);
        }

        if (any_message_receive.s_header.s_type == CS_REQUEST) {
//            printf("%zu REQUEST case started id: %zu time: %d\n", babyMaybeProcess->id, babyMaybeProcess->global_elite->sender_proc, any_message_receive.s_header.s_local_time);

            push(babyMaybeProcess, any_message_receive.s_header.s_local_time, babyMaybeProcess->global_elite->sender_proc);

            incr_lamport_time(babyMaybeProcess->global_elite);
            Message replay_message_send;
            MessageHeader replayMessageHeader;
            replayMessageHeader.s_type = CS_REPLY;
            replayMessageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
            replayMessageHeader.s_payload_len = 0;
            replayMessageHeader.s_magic = MESSAGE_MAGIC;
            replay_message_send.s_header = replayMessageHeader;

            send(babyMaybeProcess, babyMaybeProcess->global_elite->sender_proc, &replay_message_send);
        } else if (any_message_receive.s_header.s_type == CS_RELEASE) {
//            printf("%zu RELEASE case started\n", babyMaybeProcess->id);
            pop(babyMaybeProcess);
            if (babyMaybeProcess->global_elite->queue[0]->id == babyMaybeProcess->id && replays_got == babyMaybeProcess->global_elite->number_of_child_procs - 2) {
                break;
            }
        } else if (any_message_receive.s_header.s_type == CS_REPLY) {

            replays_got = replays_got + 1;

//            printf("%zu REPLY case started replays: %d\n", babyMaybeProcess->id, replays_got);

            if (babyMaybeProcess->global_elite->queue[0]->id == babyMaybeProcess->id && replays_got == babyMaybeProcess->global_elite->number_of_child_procs - 2) {
                break;
            }
        } else if (any_message_receive.s_header.s_type == DONE) {
            if (babyMaybeProcess->id == 2) {
//                printf("LOOOOOOOOOOOL proc, %zu\n", babyMaybeProcess->id);
            }
//            printf("DONE %d %zu\n", any_message_receive.s_header.s_local_time, babyMaybeProcess->global_elite->sender_proc);
//            babyMaybeProcess->global_elite->done_procs = babyMaybeProcess->global_elite->done_procs + 1;
            babyMaybeProcess->global_elite->done_procs++;
//            printf("total: %zu child %zu received message: %s\n", babyMaybeProcess->global_elite->done_procs, babyMaybeProcess->id, any_message_receive.s_payload);

//            printf("%zu DONE case started dones: %zu\n", babyMaybeProcess->id, babyMaybeProcess->global_elite->done_procs);
            if (babyMaybeProcess->global_elite->done_procs == babyMaybeProcess->global_elite->number_of_child_procs-2) {
                break;
            }
        } else {
            printf("error message type, %d\n", any_message_receive.s_header.s_type);
            return -1;
        }
    }
    return 0;
}

int release_cs(const void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;
//    printf("%zu releasing cs\n", babyMaybeProcess->id);
    pop(babyMaybeProcess);

    incr_lamport_time(babyMaybeProcess->global_elite);
    Message release_message_send;
    MessageHeader messageHeader;
    messageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
    messageHeader.s_type = CS_RELEASE;
    messageHeader.s_magic = MESSAGE_MAGIC;
    messageHeader.s_payload_len = 0;
    release_message_send.s_header = messageHeader;

    for (size_t i = 1; i < babyMaybeProcess->global_elite->number_of_child_procs; i++) {
        if (i != babyMaybeProcess->id) {
            send(babyMaybeProcess, i, &release_message_send);
//            printf("proc %zu send to %zu\n", babyMaybeProcess->id, i);
        }
    }
    return 0;
}
