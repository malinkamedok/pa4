/**
 * @file     pa1.h
 * @Author   Michael Kosyakov and Evgeniy Ivanov (ifmo.distributedclass@gmail.com)
 * @date     March, 2014
 * @brief    Constants for programming assignment 1
 *
 * Students must not modify this file!
 */

#ifndef __IFMO_DISTRIBUTED_CLASS_PA1__H
#define __IFMO_DISTRIBUTED_CLASS_PA1__H

#include "common.h"
#include "ipc.h"

/* %1d - local id, %5d - PID, e.g.
 * Process 1 (pid 12341, parent 12340) has STARTED\n
 */
static const char * const log_started_fmt =
    "Process %1d (pid %5d, parent %5d) has STARTED\n";

static const char * const log_received_all_started_fmt =
    "Process %1d received all STARTED messages\n";

static const char * const log_done_fmt =
    "Process %1d has DONE its work\n";

static const char * const log_received_all_done_fmt =
    "Process %1d received all DONE messages\n";

/* Iteration enumerated starting from 1, i.e.
 * 1, 2, 3, 4 out of 4.
 * <timestamp> process <local id> ...
 */
static const char * const log_loop_operation_fmt =
        "process %1d is doing %d iteration out of %d\n";

//------------------------------------------------------------------------------
// Functions below must be implemented by students
//------------------------------------------------------------------------------

int request_cs(const void * self);
int release_cs(const void * self);

//------------------------------------------------------------------------------
// Functions below are implemented by lector
//------------------------------------------------------------------------------
void print(const char * s);

#endif // __IFMO_DISTRIBUTED_CLASS_PA1__H
