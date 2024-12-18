#ifndef HANDLE_CPU_H 
#define HANDLE_CPU_H

#include "globals.h"
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>
#include "handle_block.h"
#include "handle_memory.h"
#include "mutex.h"
#include "planners.h"

void cpu_response_handler(); //CPU

char * syscall_code_to_string (t_operation_code syscall_code);

t_cpu_return_data * receive_cpu_return(); //CPU

void send_tcb_to_cpu(t_tcb * selected_thread); //CPU

void send_interrupt(t_tcb * tcb);//CPU

void cpu_return_data_destroy(t_cpu_return_data * cpu_return_data); //CPU

void syscall_data_destroy(t_syscall_data * syscall_data);

void tcb_and_pcb_release_handler(void * pcb_void);

void finish_process(t_pcb * pcb,t_tcb*tcb);
#endif