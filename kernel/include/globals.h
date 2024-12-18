#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <commons/log.h>
#include <commons/string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>
#include <../../utils/src/utils/process.h>


/*
Qué ocurre acá? solo estamos DECLARANDO las variables globales que compartiran los distintos modulos, algunas necesariamente deben ser variables globales
otras quizas no, no me enfoque en eso, de ultima si un modulo no usa una variable global nunca, no es la muerte de nadie.

solo estamos DECLARANDO, o sea, el compilador sabe que existen estas variables, pero no le estamos asignando espacio en el stack nunca, por lo tanto solo 
con la declaracion no podemos asignar valores, es por eso que en globals.c DEFINIMOS, o sea, hacemos que se reserve espacio en memoria, para que cada modulo que 
usa la variable global, pueda ya trabajar con un espacio de memoria determinado
*/


// CREO QUE CADA .H debe incluir este.h para conocer todos los tipos de datos y todas las variables globales

// o al menos, los tipos de datos que se usan en varios archivos, si o si 

typedef struct
{
    t_config *config;
    char *memory_ip;
    char *memory_port;
    char *cpu_ip;
    char *cpu_dispatch_port;
    char *cpu_interrupt_port;
    char *planning_algorithm;
    int quantum;
    char *log_level;
} t_kernel_config;

typedef struct{
    t_operation_code syscall_code; //sirve para manejar el orden de los elementos de la lista de parametros
    t_list * syscall_parameters; //lista de los parametros de las syscall en caso de que los haya
}t_syscall_data;
typedef struct{
    int pid;
    int tid;
    t_operation_code return_type; //nunca es nulo
    t_syscall_data * syscall_data; // si return_type = quantum => su valor es nulo, en cambio si return_type es SYSCALL, no puede ser nula
}t_cpu_return_data;

typedef struct{
    int priority;
    t_list * sublist;
}t_ready_priority_sublist;//nodo sublistas para cola ready en colas multi nivel---> igual usan el mutex de ready

typedef struct{
    int miliseconds;
    t_tcb * tcb;
}t_io_block_handler;

typedef struct{
    int pid;
    t_list * tid_list;
}t_finalized_process;

extern t_log *logger;

extern t_kernel_config *kernel_config;

extern int cpu_dispatch_connection; 
extern int cpu_interrupt_connection;
extern int memory_connection; // para handshake nomas

extern pthread_t thread_short_term;
extern pthread_t thread_long_term;
extern pthread_t thread_cpu_dispatch;
extern pthread_t planner_thread;

extern sem_t sem_counter_new;
extern sem_t sem_retry_process_creation;
extern sem_t plan_new_thread;
extern sem_t wait_cpu_response;
extern sem_t sem_counter_ready;
extern sem_t sem_counter_io_block_list;

extern sem_t sem_long_term; //sem_binary_process_handler
extern sem_t sem_short_term; //sem_ejecucion

extern uint32_t size_process_0;

extern char* path_process_0;
extern char* config_prueba;

extern t_list * new;
extern t_list * ready;
extern t_list * exitt; //finalized_processes_list
extern t_tcb * exec;

extern t_list * io_block_list;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_exec;
extern pthread_mutex_t mutex_exit; //mutex_finalized_processes_list

extern pthread_mutex_t mutex_io_block_list;

extern bool no_memory_for_new_process;//IMPORTANTISIMO: este bool NO DICE si hay espacio en memoria para crear un proceso, nada mas nos dice si el hilo que crea procesos esta bloqueado porque la ultima vez que se bloqueó no habia espacio, o sea, solo sirve para volver a intentar

extern int pid_counter;


#endif