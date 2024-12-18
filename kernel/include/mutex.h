#ifndef MUTEX_H_
#define MUTEX_H_

#include "globals.h"
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>
#include "tcb.h"
#include "planners.h"

t_mutex * create_mutex(char * mutex_name); //MUTEX

t_mutex * find_mutex_asigned_to_pcb(t_pcb * pcb, char * mutex_name); //MUTEX

bool mutex_is_taken (t_mutex * mutex); //MUTEX

void asign_mutex(t_mutex * mutex, t_tcb* tcb); // MUTEX

bool mutex_is_taken_by_tcb (t_mutex * mutex, t_tcb* tcb); // MUTEX

void free_taken_mutexes(t_pcb* pcb,t_tcb*tcb_to_finish); //MUTEX

t_mutex * find_mutex_by_blocked_thread(t_pcb * pcb, t_tcb*tcb); //MUTEX

void destroy_mutex(t_mutex* mutex);

#endif