#include <../include/planners.h>


void init_mutex(){
	pthread_mutex_init(&mutex_new,NULL);
	pthread_mutex_init(&mutex_ready,NULL);
	pthread_mutex_init(&mutex_exec,NULL);
	pthread_mutex_init(&mutex_io_block_list,NULL);
	pthread_mutex_init(&mutex_exit,NULL);
}

void init_lists(){
	new = list_create();
	ready = list_create();
	io_block_list = list_create();
	exitt = list_create();
}

void init_semaphores(){ //chequear para cada semaforo que se haga post y wait en los casos en los que teoricamente se les deberia hacer post y wait
	sem_init(&sem_counter_new,0,0);
	sem_init(&sem_retry_process_creation,0,0);
	sem_init(&plan_new_thread, 0, 1);
	sem_init(&wait_cpu_response, 0, 0);
	sem_init(&sem_counter_ready,0,0);
	sem_init(&sem_counter_io_block_list,0,0);
	sem_init(&sem_long_term,0,1);
	sem_init(&sem_short_term,0,1);
}

