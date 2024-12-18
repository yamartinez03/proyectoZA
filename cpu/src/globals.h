#ifndef CPU_GLOBALS_H_
#define CPU_GLOBALS_H_

#include <commons/log.h>
#include <commons/string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>

#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/operation_handler.h>
#include <../../utils/src/utils/serialization.h>
#include <../../utils/src/utils/sockets.h>
#include <../../utils/src/utils/utils.h>


typedef struct
{
  t_config *config;
  char *memory_ip;
  char *memory_port;
  char *listen_dispatch_port;
  char *listen_interrupt_port;
  char *log_level;
} t_cpu_config;

typedef struct{
    int pid;
    int tid;
    int time_in_cpu;//es una variable que cuenta cuantas veces se mando este thread a ejecutar--> es buena para ignorar interrupciones de quantum viejas
}t_thread;

typedef struct{
    uint32_t pc;
    uint32_t ax;
    uint32_t bx;
    uint32_t cx;
    uint32_t dx;
    uint32_t ex;
    uint32_t fx;
    uint32_t gx;
    uint32_t hx;
    uint32_t base;
    uint32_t limite;
}t_cpu_registers; //sera unico en todo el cpu

typedef struct{
    t_operation_code instruction_code;
    t_operation_code syscall_code;
    t_list * parameters_list;
}t_instruction;

typedef struct{
    int tid;
    int pid;
    int time_in_cpu;
}t_interruption; //fijate que es igual a un t_thread, es simplemente porque almacenan la misma informacion, y como solo hay interrupciones de quantum=> solo hace falta que conozcamos el tid y el pid

extern t_log *logger;
extern t_cpu_config *cpu_config;

extern int memory_connection;
extern int cliente_interrupt;
extern int cliente_dispatch;
extern int cpu_dispatch_connection;
extern int cpu_interrupt_connection;

extern sem_t sem_receive_new_thread;

extern bool kernel_replanifica;

extern bool thread_in_memory;

extern t_cpu_registers cpu_registers;

extern t_interruption current_interruption; //se sobreescribe por cada interrupcion que recibe interrupt

extern pthread_mutex_t mutex_current_interruption;

extern t_thread current_thread; //por ahora no hace falta mutex creo

#endif
