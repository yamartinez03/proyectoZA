#include "globals.h"

t_log *logger = NULL;
t_cpu_config *cpu_config = NULL;

int memory_connection = 0; // CPU SE CONECTA A MEMORIA Y MANTIENEN LA CONEXION, NO LA DESTRUYE
int cliente_interrupt = 0; // CONEXION BIDIRECCIONAL
int cliente_dispatch = 0; // CONEXION BIDIRECCIONAL
int cpu_dispatch_connection = 0; //ESCUCHA
int cpu_interrupt_connection = 0; //ESCUCHA

sem_t sem_receive_new_thread;

bool kernel_replanifica=false;

bool thread_in_memory=true;

t_list * thread_list = NULL;

t_cpu_registers cpu_registers;

t_interruption current_interruption;

pthread_mutex_t mutex_current_interruption;

t_thread current_thread;
