#include <../include/handle_memory.h>

void request_thread_creation_to_memory(t_tcb * tcb) {
	
	int new_memory_connection = create_connection(logger,"MEMORY - CREATING PROCESS",kernel_config->memory_ip,kernel_config->memory_port,TYPE_SOCKET_CLIENT);
	t_paquete * paquete = crear_paquete(THREAD_CREATE);

	t_operation_code buffer;
	agregar_a_paquete(paquete,&(tcb->pid),sizeof(int));
	agregar_a_paquete(paquete,tcb->path,string_length(tcb->path)+1);
	agregar_a_paquete(paquete,&(tcb->tid),sizeof(int));
	enviar_paquete(paquete,new_memory_connection);
	
	recv(new_memory_connection,&buffer,sizeof(t_operation_code),MSG_WAITALL);

	destroy_socket(new_memory_connection);
	
	eliminar_paquete(paquete);
}

void request_thread_release_to_memory(t_tcb* tcb){ 
	
	int new_memory_connection = create_connection(logger,"MEMORY - FINISH THREAD",kernel_config->memory_ip,kernel_config->memory_port,TYPE_SOCKET_CLIENT);
	t_operation_code buffer;
	t_paquete * paquete = crear_paquete(FINISH_THREAD);
	agregar_a_paquete(paquete,&(tcb->pid),sizeof(int));
	agregar_a_paquete(paquete,&(tcb->tid),sizeof(int));
	enviar_paquete(paquete,new_memory_connection);

	recv(new_memory_connection,&buffer,sizeof(t_operation_code),MSG_WAITALL);

	destroy_socket(new_memory_connection);
	eliminar_paquete(paquete);
	
}

bool create_process_in_memory(t_pcb* pcb){
    
    int new_memory_connection = create_connection(logger,"MEMORY - CREATING PROCESS",kernel_config->memory_ip,kernel_config->memory_port,TYPE_SOCKET_CLIENT);
    t_operation_code memory_answer;
    //enviar 
    t_paquete * paquete = crear_paquete(PROCESS_CREATE);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete,pcb->main_thread_path,strlen(pcb->main_thread_path)+1);
    agregar_a_paquete(paquete,&(pcb->size),sizeof(uint32_t));

	log_debug(logger,"Solicitando creacion del proceso %d",pcb->pid);
    enviar_paquete(paquete,new_memory_connection);

    recv(new_memory_connection,&memory_answer,sizeof(t_operation_code),MSG_WAITALL);//TODO: arreglar con los tomi y tincho como recibir la rta de memoria 

    destroy_socket(new_memory_connection);
    eliminar_paquete(paquete);

    return memory_answer==OK;
}


void request_process_release_to_memory(void * pcb_void){//TO DO
	t_pcb* pcb = (t_pcb*) pcb_void;
	t_operation_code buffer;
	int new_memory_connection = create_connection(logger,"MEMORY - FINISH PROCESS",kernel_config->memory_ip,kernel_config->memory_port,TYPE_SOCKET_CLIENT);

	t_paquete * paquete = crear_paquete(FINISH_PROCESS);
	agregar_a_paquete(paquete,&(pcb->pid),sizeof(int));
	enviar_paquete(paquete,new_memory_connection);

	recv(new_memory_connection,&buffer,sizeof(t_operation_code),MSG_WAITALL);

	destroy_socket(new_memory_connection);
	//recibo respuesta pero no me interesa
	eliminar_paquete(paquete);
}

void handle_memory_dump(void * tcb_p_void){ //falta la referencia del pcb
	t_tcb * tcb = (t_tcb*) tcb_p_void;
	t_pcb * pcb = find_pcb_from_tcb(tcb); //no puede devolver null
	t_operation_code memory_answer;

	int new_memory_connection = create_connection(logger, "MEMORY - SYSCALL DUMP MEMORY", kernel_config->memory_ip, kernel_config->memory_port, TYPE_SOCKET_CLIENT);
	t_paquete * paquete = crear_paquete(DUMP_MEMORY); //TODO: Checkear el nombre con memoria
	agregar_a_paquete(paquete, &(tcb->pid), sizeof(int)); //TODO: chequear que es lo que debe recibir memoria
	agregar_a_paquete(paquete, &(tcb->tid), sizeof(int));
	enviar_paquete(paquete, new_memory_connection);
	
	recv(new_memory_connection,&memory_answer,sizeof(t_operation_code),MSG_WAITALL);
	destroy_socket(new_memory_connection);
	
	log_debug(logger,"Respuesta de MEMORY DUMP recibida para (%d,%d), esperando semaforo",tcb->pid,tcb->tid);
	sem_wait(&sem_short_term);
	log_debug(logger,"Procesando respuesta de memoria por MEMORY DUMP de (%d,%d)",tcb->pid,tcb->tid);
	
	if(memory_answer == ERROR) {
		log_debug(logger,"(%d,%d) falló por MEMORY DUMP",tcb->pid,tcb->tid);
		if(pcb->state == EXIT){
			log_debug(logger,"Sin embargo, el proceso %d ya habia finalizado, entonces ignoro el error",pcb->pid);
			sem_post(&(tcb->sem_finished_thread));
		}else if(tcb->state==EXIT){
			log_debug(logger,"(%d,%d) tenia una solicitud de CANCELACIÓN y falló por MEMORY DUMP: igual finalizamos el proceso",tcb->pid,tcb->tid);
			sem_post(&(tcb->sem_finished_thread));
			finish_process(pcb,tcb);
		}else{
			log_debug(logger,"(%d,%d) falló por MEMORY DUMP",tcb->pid,tcb->tid);
			change_block_type(tcb,INSTRUCTION);
			remove_tcb_from_state(tcb); //lo quito de block , queda en null state
			handle_sending_tcb_to_exit(pcb,tcb);
			finish_process(pcb,tcb);
		}
		
	} else {
		log_debug(logger,"(%d,%d) tuvo exito al hacer MEMORY DUMP",tcb->pid,tcb->tid);
		if(pcb->state == EXIT){
			log_debug(logger,"Sin embargo, el proceso %d ya habia finalizado",pcb->pid);
			sem_post(&(tcb->sem_finished_thread));
		}else if(tcb->state==EXIT){
			log_debug(logger,"(%d,%d) tenia una solicitud de CANCELACIÓN, entonces no meto a READY de nuevo",tcb->pid,tcb->tid);
			sem_post(&(tcb->sem_finished_thread));
		}else{
			log_debug(logger,"(%d,%d) tuvo exito al hacer MEMORY DUMP, y como no se pidio finalizar, vuelve a READY tranquilamente",tcb->pid,tcb->tid);
			change_block_type(tcb,INSTRUCTION);
			remove_tcb_from_state(tcb);
			add_tcb_to_state(tcb,READY);
		}

	}

	eliminar_paquete(paquete);

	sem_post(&sem_short_term);
	
	 
}
