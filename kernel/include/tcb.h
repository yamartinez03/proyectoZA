#ifndef TCB_H
#define TCB_H

#include "globals.h"
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>
#include "pcb.h"
#include "planners.h"
#include "short_term.h"

t_tcb * create_tcb(t_pcb * pcb,char * thread_path,int thread_priority);//TCB

void change_state( t_tcb * tcb, t_state new_state); //TCB

void add_tcb_to_state(t_tcb * tcb,t_state new_state);//TCB

void remove_tcb_from_state(t_tcb* tcb);//TCB

t_tcb* find_tcb(t_pcb * pcb, int tid); //TCB

void change_block_type(t_tcb* tcb,t_operation_code new_block_type); //TCB

void joined_threads_to_ready(t_tcb * tcb_to_finish); // TCB

void handle_sending_tcb_to_exit(t_pcb*pcb,t_tcb * tcb_to_finish); 

t_list * find_all_tcbs_of_pcb(t_pcb * pcb); //TCBS

void destroy_tcb(t_tcb*tcb);

#endif