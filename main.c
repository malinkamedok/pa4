#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "pa1.h"
#include "pipes.h"
#include "ipc.h"
#include "lamport.h"
#include <fcntl.h>

int main(int argc, char **argv) {

    int mutex_flag = 0;

    if (argc < 3 || argc > 4) {
        return -1;
    }

    if (argc == 4) {
        if (strcmp(argv[3], "--mutexl") == 0) {
            mutex_flag = 1;
        } else {
            return -1;
        }
    }

    printf("mutex status: %d\n", mutex_flag);

    if (strcmp(argv[1], "-p") != 0) {
        return -1;
    }

    int number_of_child_procs = atoi(argv[2]);
    if (number_of_child_procs == 0) {
        return -1;
    }

    if (number_of_child_procs < 1 || number_of_child_procs > 10) {
        return -1;
    }

    FILE *pi;
    pi = fopen("pipes.log", "a+");

    FILE *ev;
    ev = fopen("events.log", "a+");

    pipes_all_global * global = new(number_of_child_procs+1);

    //pipes creation
    for (size_t i = 0; i < number_of_child_procs + 1; i++) {
        for (size_t j = 0; j < number_of_child_procs + 1; j++) {
            if (i != j) {
                int result_pipe = pipe(global->pipes_all[i][j].fd);
                fprintf(pi, "pipe opened i: %zu j: %zu\n", i, j);
                fcntl(global->pipes_all[i][j].fd[0], F_SETFL, O_NONBLOCK);
                fcntl(global->pipes_all[i][j].fd[1], F_SETFL, O_NONBLOCK);
                if (result_pipe == -1) {
                    return -1;
                }
            }
        }
    }

    init_lamport_time(global);

    size_t process_number = 0;

    for (size_t count = 1; count < number_of_child_procs+1; count++) {
        if (fork() == 0) {
            process_number = count;
            printf(log_started_fmt, (int ) process_number, getpid(), getppid());
            fprintf(ev, log_started_fmt, (int ) process_number, getpid(), getppid());

            //CLOSING PIPES FOR CHILD PROC
            // close pipes not working with current process
            for (size_t i = 0; i < number_of_child_procs+1; i++) {
                for (size_t j = 0; j < number_of_child_procs+1; j++) {
                    if (i != j && i != process_number && j != process_number) {
                        close(global->pipes_all[i][j].fd[0]);
                        close(global->pipes_all[i][j].fd[1]);
                        close(global->pipes_all[j][i].fd[0]);
                        close(global->pipes_all[j][i].fd[1]);
                    }
                }
            }
            for (size_t i = 0; i < number_of_child_procs+1; i++) {
                if (i != process_number) {
                    close(global->pipes_all[process_number][i].fd[0]);
                    close(global->pipes_all[i][process_number].fd[1]);
                }
            }
            baby_maybe_process * babyMaybeProcess = new_baby(process_number, global);

            incr_lamport_time(global);

            //SENDING STARTED MESSAGE
            char started_message[49];
            sprintf(started_message, log_started_fmt, (int ) process_number, getpid(), getppid());
            Message message_start_send;
            MessageHeader messageHeader;
            messageHeader.s_magic = MESSAGE_MAGIC;
            messageHeader.s_payload_len = 49;
            messageHeader.s_local_time = read_lamport_time(global);
            messageHeader.s_type = STARTED;
            message_start_send.s_header = messageHeader;
            for (size_t i = 0; i < 49; i++) {
                message_start_send.s_payload[i] = started_message[i];
            }

            send_multicast((void*)babyMaybeProcess, &message_start_send);

            //READING STARTED MESSAGE
            for (size_t i = 1; i < number_of_child_procs+1; i++) {
                if (i != process_number) {
                    // printf("child proc %zu try read msg from %zu use des %d\n", process_number, i, global->pipes_all[i][process_number].fd[0]);
                    Message message_start_receive;

                    receive((void*)babyMaybeProcess, i, &message_start_receive);
                    printf("child %zu received message: %s\n", count, message_start_receive.s_payload);

                    if (message_start_receive.s_header.s_local_time > read_lamport_time(global)) {
                        set_lamport_time(global, message_start_receive.s_header.s_local_time);
                        incr_lamport_time(global);
                    } else {
                        incr_lamport_time(global);
                    }
                }
            }

//            for (size_t i = 1; i < number_of_child_procs+1; i++) wait(NULL);

            //log_received_all_started_fmt
            printf(log_received_all_started_fmt, (int ) process_number);
            fprintf(ev, log_received_all_started_fmt, (int ) process_number);

            //WORK WITHOUT MUTEX
            size_t iterations = 5 * process_number;
            if (mutex_flag == 0) {
                for (size_t i = 1; i < iterations+1; i++) {
                    char print_log[100];
                    sprintf(print_log, log_loop_operation_fmt, (int )process_number, (int )i, (int )iterations);
                    print(print_log);
                }
            }

            //WORK WITH MUTEX
            if (mutex_flag == 1) {
                //request cs

                sleep(1);

                int zulepa = request_cs(babyMaybeProcess);
                printf("oshibka: %d\n", zulepa);

                //main work

                //release cs
            }

            //log_done_fmt
            char done_message[29];
            sprintf(done_message, log_done_fmt, (int) process_number);
            printf(log_done_fmt, (int) process_number);
            fprintf(ev, log_done_fmt, (int) process_number);

            //SENDING DONE MESSAGE
            incr_lamport_time(global);

            Message message_done_send;
            MessageHeader messageDoneHeader;
            messageDoneHeader.s_magic = MESSAGE_MAGIC;
            messageDoneHeader.s_payload_len = 29;
            messageDoneHeader.s_local_time = read_lamport_time(global);
            messageDoneHeader.s_type = DONE;
            message_done_send.s_header = messageDoneHeader;
            for (size_t i = 0; i < 29; i++) {
                message_done_send.s_payload[i] = done_message[i];
            }
            send_multicast((void*)babyMaybeProcess, &message_done_send);


            for (size_t j = 0; j < number_of_child_procs + 1; j++) {
                if (j != process_number) {
                    close(global->pipes_all[process_number][j].fd[1]);
//                    printf("pid %d, i: %zu, j: %zu DONE message printed\n", getpid(), i, j);
                }
            }


//            for (size_t i = 1; i < number_of_child_procs + 1; i++) wait(NULL);

            //READING DONE MESSAGE
            for (size_t i = 1; i < number_of_child_procs+1; i++) {
                if (i != process_number) {
                    Message messageDoneReceive;

                    receive((void*)babyMaybeProcess, i, &messageDoneReceive);

                    close(global->pipes_all[i][process_number].fd[0]);
                    printf("child %zu received message: %s from proc #%zu\n", count, messageDoneReceive.s_payload, i);
                    if (messageDoneReceive.s_header.s_local_time >= read_lamport_time(global)) {
                        set_lamport_time(global, messageDoneReceive.s_header.s_local_time);
                        incr_lamport_time(global);
                    } else {
                        incr_lamport_time(global);
                    }
                }
            }

            //log_received_all_done_fmt
            printf(log_received_all_done_fmt, (int ) process_number);
            fprintf(ev, log_received_all_done_fmt, (int ) process_number);

            exit(0);
        }
    }

    //CLOSING PIPES FOR PARENT PROC
    for (size_t i = 0; i < number_of_child_procs+1; i++) {
        for (size_t j = 0; j < number_of_child_procs+1; j++) {
            if (i != j && i != 0 && j != 0) {
                close(global->pipes_all[i][j].fd[0]);
                close(global->pipes_all[i][j].fd[1]);
                close(global->pipes_all[j][i].fd[0]);
                close(global->pipes_all[j][i].fd[1]);
            }
        }
    }
    for (size_t i = 0; i < number_of_child_procs+1; i++) {
        if (i != process_number) {
            close(global->pipes_all[process_number][i].fd[0]);
            close(global->pipes_all[i][process_number].fd[1]);
        }
    }

    for (size_t count = 1; count < number_of_child_procs+1; count++) wait(NULL);

    //READING STARTED MESSAGE BY PARENT
    for (size_t i = 1; i < number_of_child_procs+1; i++) {
        Message message;
        read(global->pipes_all[i][0].fd[0], &message.s_header, sizeof(MessageHeader));
        read(global->pipes_all[i][0].fd[0], message.s_payload, message.s_header.s_payload_len);

        if (message.s_header.s_local_time >= read_lamport_time(global)) {
            set_lamport_time(global, message.s_header.s_local_time);
            incr_lamport_time(global);
        } else {
            incr_lamport_time(global);
        }

        printf("parent received: %s from proc #%zu\n", message.s_payload, i);
    }

    //READING DONE MESSAGE BY PARENT
    for (size_t i = 1; i < number_of_child_procs+1; i++) {
        Message message;
        read(global->pipes_all[i][0].fd[0], &message.s_header, sizeof(MessageHeader));
        read(global->pipes_all[i][0].fd[0], message.s_payload, message.s_header.s_payload_len);
        close(global->pipes_all[i][0].fd[0]);

        if (message.s_header.s_local_time >= read_lamport_time(global)) {
            set_lamport_time(global, message.s_header.s_local_time);
            incr_lamport_time(global);
        } else {
            incr_lamport_time(global);
        }

        printf("parent received: %s from proc #%zu\n", message.s_payload, i);
    }

    fclose(ev);
    fclose(pi);

    return 0;
}
