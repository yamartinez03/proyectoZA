#include <../include/globals.h>

//aca hacemos que las variables globales tengan un espacio definido en memoria, de una vez x todas

int cpu_dispatch_connection=0;		
int cpu_interrupt_connection=0;		
int memory_connection=0;
int pid_counter=0;			

pthread_t thread_short_term=0;
pthread_t thread_long_term=0;
pthread_t thread_cpu_dispatch=0;
pthread_t planner_thread=0;

t_kernel_config* kernel_config=NULL;	

t_log *logger=NULL;	

sem_t sem_counter_new;
sem_t sem_retry_process_creation;
sem_t plan_new_thread;
sem_t wait_cpu_response;
sem_t sem_counter_ready;
sem_t sem_counter_io_block_list;

sem_t sem_long_term;
sem_t sem_short_term;

char* path_process_0=NULL;
char* config_prueba=NULL;

uint32_t size_process_0=0;

t_list* new=NULL;
t_list* ready=NULL;
t_list* exitt=NULL; //finalized_processes_list
t_tcb * exec=NULL;

t_list* io_block_list=NULL;

pthread_mutex_t mutex_new; 
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit; //mutex_finalized_processes_list

pthread_mutex_t mutex_io_block_list;



bool no_memory_for_new_process=false; 



