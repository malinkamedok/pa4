#include "pipes.h"

pipes_all_global * new(size_t number) {
    pipes_all_global * glb = (pipes_all_global *) malloc(sizeof(pipes_all_global));
    glb->number_of_child_procs = number;
    glb->pipes_all = (struct pipes **) malloc(sizeof(struct pipes *) * glb->number_of_child_procs);
    for (size_t a = 0; a < glb->number_of_child_procs; ++a) {
        glb->pipes_all[a] = (struct pipes *)  malloc(sizeof(struct pipes) * glb->number_of_child_procs);
    }
    return glb;
}


baby_maybe_process * new_baby(size_t id, pipes_all_global * x) {
    baby_maybe_process * mb = (baby_maybe_process *) malloc(sizeof(baby_maybe_process));
    mb->id = id;
    mb->global_elite = x;
    return mb;
}

request_queue * new_member_in_queue(timestamp_t time, size_t id) {
    request_queue * rq = (request_queue *) malloc(sizeof(request_queue));
    rq->request_time = time;
    rq->id = id;
    return rq;
}
