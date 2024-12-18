#ifndef PCB_H
#define PCB_H

#include "globals.h"
//#include "kernel.h" //esto es necesario? creo que no
#include <commons/collections/queue.h>
#include "mutex.h"
//#include "semaphore.h" //ya en globals, podría sacarse de aqui?
#include <../../utils/src/utils/operation_handler.h>// probar si anda con #include "operation_handler.h", porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>

t_pcb*create_pcb(uint32_t size,char* main_thread_path,int main_thread_priority); //PCB

int asign_pid(); //PCB

int asign_tid_from_pcb(t_pcb * pcb);//PCB

t_pcb * find_pcb_from_tcb(t_tcb * tcb);//pcb

void change_pcb_state(t_pcb*pcb,t_state state);

void destroy_pcb(t_pcb*pcb);

t_finalized_process * create_finalized_process(t_pcb*pcb);

#endif