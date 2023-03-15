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
        if (strcmp(argv[1], "--mutexl") == 0) {
            mutex_flag = 1;
        } else {
            return -1;
        }
    }

//    printf("mutex status: %d\n", mutex_flag);

    int number_of_child_procs = 0;
    if (mutex_flag == 1) {
        number_of_child_procs = atoi(argv[3]);
        if (number_of_child_procs == 0) {
            return -1;
        }
        if (strcmp(argv[2], "-p") != 0) {
            return -1;
        }
    }

    if (mutex_flag == 0) {
        number_of_child_procs = atoi(argv[2]);
        if (number_of_child_procs == 0) {
            return -1;
        }
        if (strcmp(argv[1], "-p") != 0) {
            return -1;
        }
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
//            printf(log_started_fmt, (int ) process_number, getpid(), getppid());
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
            babyMaybeProcess->global_elite->queue_size = 0;
            babyMaybeProcess->global_elite->done_procs = 0;

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

                    int receive_result = -1;
                    while (receive_result == -1) {
                        receive_result = receive((void*)babyMaybeProcess, i, &message_start_receive);
                        if (message_start_receive.s_header.s_type == STARTED) {

                        } else {
                            receive_result = -1;
                        }
                    }
//                    printf("child %zu received message: %s\n", count, message_start_receive.s_payload);

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
//            printf(log_received_all_started_fmt, (int ) process_number);
            fprintf(ev, log_received_all_started_fmt, (int ) process_number);

            size_t iterations = 5 * process_number;

            //WORK WITHOUT MUTEX
            if (mutex_flag == 0) {
                for (size_t i = 1; i < iterations+1; i++) {
                    char print_log[100];
                    sprintf(print_log, log_loop_operation_fmt, (int )process_number, (int )i, (int )iterations);
                    print(print_log);
                }
            }

            //WORK WITH MUTEX
            if (mutex_flag == 1) {
                sleep(1);

                for (size_t i = 1; i < iterations+1; i++) {
//                    int zulepa = request_cs(babyMaybeProcess);
//                    printf("oshibka: %d\n", zulepa);
                    request_cs(babyMaybeProcess);

                    char print_log[100];
                    sprintf(print_log, log_loop_operation_fmt, (int )process_number, (int )i, (int )iterations);
                    print(print_log);
//                    printf(log_loop_operation_fmt, (int )process_number, (int )i, (int )iterations);

//                    printf("%zu print\n", process_number);

                    release_cs(babyMaybeProcess);

//                    if (process_number != number_of_child_procs && global->done_procs != number_of_child_procs-1) {
//                        release_cs(babyMaybeProcess);
//                    }
                }
//                sleep(1);
            }

            //log_done_fmt
            char done_message[29];
            sprintf(done_message, log_done_fmt, (int) process_number);
//            printf(log_done_fmt, (int) process_number);
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
            for (size_t i = 0; i < number_of_child_procs+1; i++) {
                if (i != process_number) {
                    send(babyMaybeProcess, i, &message_done_send);
//                    printf("%zu DONE to proc %zu\n", process_number, i);
                }
            }

//            if (process_number == number_of_child_procs) {
//                Message done;
//                MessageHeader messageHeader1;
//                messageHeader1.s_local_time = read_lamport_time(global);
//                messageHeader1.s_type = DONE;
//                messageHeader1.s_payload_len = 0;
//                messageHeader1.s_magic = MESSAGE_MAGIC;
//                done.s_header = messageHeader1;
//
//                for (size_t i = 0; i < number_of_child_procs; i++) {
//                    send(babyMaybeProcess, i, &done);
//                }
//            }
//            send_multicast((void*)babyMaybeProcess, &message_done_send);


//            for (size_t j = 0; j < number_of_child_procs + 1; j++) {
//                if (j != process_number) {
//                    close(global->pipes_all[process_number][j].fd[1]);
////                    printf("pid %d, i: %zu, j: %zu DONE message printed\n", getpid(), i, j);
//                }
//            }


//            for (size_t i = 1; i < number_of_child_procs + 1; i++) wait(NULL);

//            int zal = 0;
            if (process_number != number_of_child_procs) {
                //READING DONE MESSAGE
                while (babyMaybeProcess->global_elite->done_procs != number_of_child_procs-1) {
                    Message messageDoneReceive;

                    int receive_result = -1;
                    while (receive_result == -1) {
                        receive_result = receive_any(babyMaybeProcess, &messageDoneReceive);
                    }

                    if (messageDoneReceive.s_header.s_type == DONE) {
                        if (babyMaybeProcess->id == 2) {
//                            printf("LOOOOOOOOOOOL proc, %zu\n", babyMaybeProcess->id);
                        }
                        babyMaybeProcess->global_elite->done_procs = babyMaybeProcess->global_elite->done_procs + 1;
//                        printf("total: %zu child %zu received message: %s\n", babyMaybeProcess->global_elite->done_procs, count, messageDoneReceive.s_payload);
                        if (messageDoneReceive.s_header.s_local_time >= read_lamport_time(global)) {
                            set_lamport_time(global, messageDoneReceive.s_header.s_local_time);
                            incr_lamport_time(global);
                        } else {
                            incr_lamport_time(global);
                        }
                    } else if (messageDoneReceive.s_header.s_type == CS_REQUEST) {
//                    printf("%zu received req message after done\n", process_number);
                        if (messageDoneReceive.s_header.s_local_time >= read_lamport_time(global)) {
                            set_lamport_time(global, messageDoneReceive.s_header.s_local_time);
                            incr_lamport_time(global);
                        } else {
                            incr_lamport_time(global);
                        }

                        incr_lamport_time(global);
                        Message message_reply_send;
                        MessageHeader messageHeader1;
                        messageHeader1.s_local_time = read_lamport_time(global);
                        messageHeader1.s_type = CS_REPLY;
                        messageHeader1.s_payload_len = 0;
                        messageHeader1.s_magic = MESSAGE_MAGIC;
                        message_reply_send.s_header = messageHeader1;

                        send(babyMaybeProcess, babyMaybeProcess->global_elite->sender_proc, &message_reply_send);
                    }
////                    else if (messageDoneReceive.s_header.s_type == CS_RELEASE && global->sender_proc == number_of_child_procs && global->done_procs == number_of_child_procs-2) {
//////                        zal++;
//////                        if (zal < 6) {
//////                            continue;
//////                        } else {
//////                            break;
//////                        }
////                        Message rec;
////                        MessageHeader recH;
////                        read(global->pipes_all[number_of_child_procs][process_number].fd[0], &recH, sizeof(messageHeader));
////                        read(global->pipes_all[number_of_child_procs][process_number].fd[0], &rec, recH.s_payload_len);
////                        printf("read res: %s\n", rec.s_payload);
//                    else if (global->sender_proc == number_of_child_procs && global->done_procs == number_of_child_procs-2) {
//                        if (process_number == 5 || process_number == 6 || process_number == 7 || process_number == 8)
//                            sleep(1);
//                        Message rec;
//                        MessageHeader recH;
//                        read(global->pipes_all[number_of_child_procs][process_number].fd[0], &recH, sizeof(messageHeader));
//                        read(global->pipes_all[number_of_child_procs][process_number].fd[0], &rec, recH.s_payload_len);
//                        if (recH.s_type == DONE) {
//                            babyMaybeProcess->global_elite->done_procs++;
//                        }
//                        printf("%zu read res: %s\n", process_number, rec.s_payload);
//                    }
//                    else if (global->sender_proc == number_of_child_procs && global->done_procs == number_of_child_procs-2) {
//                        Message message;
//                        receive_any(babyMaybeProcess, &message);
//                        printf("%zu message: %s\n", process_number, message.s_payload);
//                        babyMaybeProcess->global_elite->done_procs++;
//                    }
                }
                //log_received_all_done_fmt
//                printf(log_received_all_done_fmt, (int ) process_number);
                fprintf(ev, log_received_all_done_fmt, (int ) process_number);
            }

            if (process_number == number_of_child_procs) {
                //log_received_all_done_fmt
//                printf(log_received_all_done_fmt, (int ) process_number);
                fprintf(ev, log_received_all_done_fmt, (int ) process_number);
            }

            for (size_t i = 1; i < number_of_child_procs+1; i++) {
                if (i != process_number) {
                    close(global->pipes_all[i][process_number].fd[0]);
                }
            }



            exit(0);
        }
    }

    baby_maybe_process * babyMaybeProcess = new_baby(process_number, global);

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
//        read(global->pipes_all[i][0].fd[0], &message.s_header, sizeof(MessageHeader));
//        read(global->pipes_all[i][0].fd[0], message.s_payload, message.s_header.s_payload_len);

        int receive_started_result = -1;
        while (receive_started_result == -1) {
            receive_started_result = receive(babyMaybeProcess, i, &message);
        }

        if (message.s_header.s_local_time >= read_lamport_time(global)) {
            set_lamport_time(global, message.s_header.s_local_time);
            incr_lamport_time(global);
        } else {
            incr_lamport_time(global);
        }

//        printf("parent received: %s from proc #%zu\n", message.s_payload, i);
    }

    //READING DONE MESSAGE BY PARENT
    for (size_t i = 1; i < number_of_child_procs+1; i++) {
        Message message;
//        read(global->pipes_all[i][0].fd[0], &message.s_header, sizeof(MessageHeader));
//        read(global->pipes_all[i][0].fd[0], message.s_payload, message.s_header.s_payload_len);

        int receive_done_result = -1;
        while (receive_done_result == -1) {
            receive_done_result = receive(babyMaybeProcess, i, &message);
        }

//        printf("%d msg type: %d time: %d\n", receive_done_result, message.s_header.s_type, message.s_header.s_local_time);

        close(global->pipes_all[i][0].fd[0]);

        if (message.s_header.s_local_time >= read_lamport_time(global)) {
            set_lamport_time(global, message.s_header.s_local_time);
            incr_lamport_time(global);
        } else {
            incr_lamport_time(global);
        }

//        printf("parent received: %s from proc #%zu\n", message.s_payload, i);
    }

    fclose(ev);
    fclose(pi);

    return 0;
}
