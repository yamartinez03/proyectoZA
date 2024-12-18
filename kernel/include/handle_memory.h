#ifndef HANDLE_MEMORY_H
#define HANDLE_MEMORY_H

#include "globals.h"
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>
#include "handle_cpu.h"
#include "planners.h"

void request_thread_creation_to_memory(t_tcb * tcb); // MEMORIA

void request_thread_release_to_memory(t_tcb* tcb); //MEMORIA

bool create_process_in_memory(t_pcb* pcb);

void request_process_release_to_memory(void * pcb_void); //MEMORIA

void handle_memory_dump(void * tcb_p_void);
#endif