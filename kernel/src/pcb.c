#include <../include/pcb.h>

t_pcb * create_pcb(uint32_t size,char* main_thread_path,int main_thread_priority){//el path recibido esta en malloc, seria
	t_pcb * new_process = malloc(sizeof(t_pcb));
	new_process->pid=asign_pid();
	new_process->state=NEW;
	new_process->main_thread_priority=main_thread_priority;
	new_process->tid_list=list_create();
	new_process->mutex_list=list_create();
	new_process->main_thread_path = main_thread_path;
	new_process->tid_counter=0; //arranca en 0 porque tiene 0 HILOS creados todavía -> el siguiente sera el 0
	new_process->size = size;
	new_process->tcb_list=list_create();
	pthread_mutex_init(&(new_process->pcb_access_mutex),NULL);
	log_debug(logger,"PCB created - PID: %d - SIZE %u - MAIN PATH: %s", new_process->pid, new_process->size, new_process->main_thread_path);
	log_info(logger,"## (%d:0) Se crea el proceso - Estado: NEW", new_process->pid);
	return new_process;
}

int asign_pid(){
	int placeholder = pid_counter;
	pid_counter++;
	return placeholder;
}

int asign_tid_from_pcb(t_pcb * pcb){
	int placeholder;
	placeholder = pcb->tid_counter;
	pcb->tid_counter++;
	return placeholder;
}

t_pcb * find_pcb_from_tcb(t_tcb * tcb){
	return tcb->pcb; //ojo puede devolver NULL si no lo encuentra 
}

void change_pcb_state(t_pcb*pcb,t_state state){
	pcb->state=state;
}

void destroy_pcb(t_pcb*pcb){
	list_destroy(pcb->tid_list); //IMPORTANTE NO DESTRUIR LUEGO LOS ELEMENTOS DE LA TID LIST -> FIJATE CREATE_FINALIZED_PROCESS
	list_destroy_and_destroy_elements(pcb->mutex_list,(void*)destroy_mutex);
	//free(pcb->main_thread_path); NO HAY QUE HACER ESTO PORQUE YA LO LIBERO EL MAIN THREAD
	//NO DESTRUIMOS TCB_LIST, ya fue destruida antes
	pthread_mutex_destroy(&(pcb->pcb_access_mutex));
	free(pcb);
}


t_finalized_process * create_finalized_process(t_pcb*pcb){
	t_finalized_process * finalized_process = malloc(sizeof(t_finalized_process));

	finalized_process->pid=pcb->pid;
	finalized_process->tid_list = list_duplicate(pcb->tid_list); //IMPORTANTE NO DESTRUIR LUEGO LOS ELEMENTOS DE LA TID LIST ORIGINAL en el DESTROY PCB

	return finalized_process;
}