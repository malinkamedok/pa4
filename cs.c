#include "pa1.h"
#include "pipes.h"
#include "lamport.h"
#include <stdio.h>
#include <unistd.h>

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

    send_multicast(babyMaybeProcess, &request_message);

    request_queue * requestQueue = new_member_in_queue(read_lamport_time(babyMaybeProcess->global_elite), babyMaybeProcess->id);
    babyMaybeProcess->global_elite->queue[0] = requestQueue;

    printf("time: %d id: %zu\n", babyMaybeProcess->global_elite->queue[0]->request_time, babyMaybeProcess->global_elite->queue[0]->id);

    sleep(1);

    Message any_message_receive;
    int replays_got, requests_got = 0;
    while (1) {
        while (receive_any(babyMaybeProcess, &any_message_receive) == -1) {}
        incr_lamport_time(babyMaybeProcess->global_elite);
        switch (any_message_receive.s_header.s_type) {
            case CS_REQUEST:
            {
                request_queue * request = new_member_in_queue(any_message_receive.s_header.s_local_time, atoi(any_message_receive.s_payload));
                requests_got = requests_got + 1;
                babyMaybeProcess->global_elite->queue[requests_got] = request;
                printf("%zu got in queue time: %d id: %zu\n", babyMaybeProcess->id, babyMaybeProcess->global_elite->queue[requests_got]->request_time, babyMaybeProcess->global_elite->queue[requests_got]->id);

                incr_lamport_time(babyMaybeProcess->global_elite);
                Message replay_message_send;
                MessageHeader replayMessageHeader;
                replayMessageHeader.s_type = CS_REPLY;
                replayMessageHeader.s_local_time = read_lamport_time(babyMaybeProcess->global_elite);
                replayMessageHeader.s_payload_len = 0;
                replayMessageHeader.s_magic = MESSAGE_MAGIC;
                replay_message_send.s_header = replayMessageHeader;

                send(babyMaybeProcess, atoi(any_message_receive.s_payload), &replay_message_send);
            }
            case CS_RELEASE:
            {
                break;
            }
            case CS_REPLY:
            {
                replays_got = replays_got + 1;
                if (replays_got == babyMaybeProcess->global_elite->number_of_child_procs - 1) {
                    break;
                }
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
    return 0;
}
