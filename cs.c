#include "pa1.h"
#include "pipes.h"
#include "lamport.h"
#include <stdio.h>
#include <unistd.h>


int compare_elements (request_queue * element0, request_queue * element1) {

    if (element0->request_time < element1->request_time) {
        return -1;
    } else if (element0->request_time > element1->request_time) {
        return 1;
    }

    if (element0->request_time == element1->request_time) {
        if (element0->id < element1->id) {
            return -1;
        } else if (element0->id > element1->id) {
            return 1;
        }
    }

    if (element0->id == element1->id) {
        printf("error in id in sorting queue\n");
        return 0;
    }

    printf("smth went wrong\n");
    return 0;
}

void sort_elements (void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    qsort(babyMaybeProcess->global_elite->queue, babyMaybeProcess->global_elite->queue_size, sizeof(request_queue), (int(*) (const void *, const void *)) compare_elements);
    printf("sort elements\n");
    for (size_t i =0; i < babyMaybeProcess->global_elite->queue_size; i++) {
        printf("%zu sorted array: %d %zu\n", babyMaybeProcess->id, babyMaybeProcess->global_elite->queue[i]->request_time, babyMaybeProcess->global_elite->queue[i]->id);
    }
}

void push (void * self, timestamp_t time, size_t id) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    request_queue * requestQueue = new_member_in_queue(time, id);
    babyMaybeProcess->global_elite->queue[babyMaybeProcess->global_elite->queue_size] = requestQueue;

    babyMaybeProcess->global_elite->queue_size = babyMaybeProcess->global_elite->queue_size + 1;

    sort_elements(babyMaybeProcess);
    return;
}

request_queue * pop (void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    request_queue * element = babyMaybeProcess->global_elite->queue[0];

    for (size_t i = 0; i < babyMaybeProcess->global_elite->queue_size; i++) {
        babyMaybeProcess->global_elite->queue[i] = babyMaybeProcess->global_elite->queue[i+1];
    }

    babyMaybeProcess->global_elite->queue_size = babyMaybeProcess->global_elite->queue_size - 1;

    return element;
}

int request_cs(const void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;

    incr_lamport_time(babyMaybeProcess->global_elite);

    char id[2];
    sprintf(id, "%zu", babyMaybeProcess->id);
    Message request_message;
    MessageHeader messageHeader;
    messageHeader.s_type = CS_REQUEST;
    messageHeader.s_magic = MESSAGE_MAGIC;
    messageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
    messageHeader.s_payload_len = 2;
    request_message.s_header = messageHeader;
    for (size_t i = 0; i < 2; i++) {
        request_message.s_payload[i] = id[i];
    }

    for (size_t i = 1; i < babyMaybeProcess->global_elite->number_of_child_procs+1; i++) {
        send(babyMaybeProcess, i, &request_message);
    }

    push(babyMaybeProcess, babyMaybeProcess->global_elite->lamport_time, babyMaybeProcess->id);

    printf("time: %d id: %zu\n", babyMaybeProcess->global_elite->queue[0]->request_time, babyMaybeProcess->global_elite->queue[0]->id);

    sleep(1);

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
        switch (any_message_receive.s_header.s_type) {
            case CS_REQUEST:
            {
//                request_queue * request = new_member_in_queue(any_message_receive.s_header.s_local_time, atoi(any_message_receive.s_payload));
//                babyMaybeProcess->global_elite->queue[requests_got] = request;
//                printf("%zu got in queue time: %d id: %zu\n", babyMaybeProcess->id, babyMaybeProcess->global_elite->queue[requests_got]->request_time, babyMaybeProcess->global_elite->queue[requests_got]->id);

                push(babyMaybeProcess, any_message_receive.s_header.s_local_time, atoi(any_message_receive.s_payload));

                incr_lamport_time(babyMaybeProcess->global_elite);
                Message replay_message_send;
                MessageHeader replayMessageHeader;
                replayMessageHeader.s_type = CS_REPLY;
                replayMessageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
                replayMessageHeader.s_payload_len = 0;
                replayMessageHeader.s_magic = MESSAGE_MAGIC;
                replay_message_send.s_header = replayMessageHeader;

                send(babyMaybeProcess, atoi(any_message_receive.s_payload), &replay_message_send);
                break;
            }
            case CS_RELEASE:
            {
                pop(babyMaybeProcess);
                if (babyMaybeProcess->global_elite->queue[0]->id == babyMaybeProcess->id && replays_got == babyMaybeProcess->global_elite->number_of_child_procs - 2) {
                    return 0;
                }
                break;
            }
            case CS_REPLY:
            {
                replays_got = replays_got + 1;
                printf("case REPLY started, replays got: %d\n", replays_got);
                printf("%zu should break now\n", babyMaybeProcess->global_elite->number_of_child_procs);
                if (babyMaybeProcess->global_elite->queue[0]->id == babyMaybeProcess->id && replays_got == babyMaybeProcess->global_elite->number_of_child_procs - 2) {
                    printf("zalupa\n");
                    return 0;
                }
                break;
            }
            case DONE:
            {
                printf("child %zu received message: %s\n", babyMaybeProcess->id, any_message_receive.s_payload);
                babyMaybeProcess->global_elite->done_procs = babyMaybeProcess->global_elite->done_procs + 1;
                if (babyMaybeProcess->global_elite->done_procs == babyMaybeProcess->global_elite->number_of_child_procs-1) {
                    return 0;
                }
                break;
            }
            default:
            {
                printf("error message type, %d\n", any_message_receive.s_header.s_type);
                return -1;
            }
        }

    }

    return 0;
}

int release_cs(const void * self) {
    baby_maybe_process * babyMaybeProcess = (baby_maybe_process*) self;
    pop(babyMaybeProcess);

    incr_lamport_time(babyMaybeProcess->global_elite);
    Message release_message_send;
    MessageHeader messageHeader;
    messageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
    messageHeader.s_type = CS_RELEASE;
    messageHeader.s_magic = MESSAGE_MAGIC;
    messageHeader.s_payload_len = 0;
    release_message_send.s_header = messageHeader;

    for (size_t i = 1; i < babyMaybeProcess->global_elite->number_of_child_procs+1; i++) {
        send(babyMaybeProcess, i, &release_message_send);
    }
    return 0;
}
