#include <../include/mutex.h>

t_mutex * create_mutex(char * mutex_name){ //crea un duplicado en el heap
	t_mutex * new_mutex = malloc(sizeof(t_mutex));
	
	new_mutex->tcb=NULL;
	new_mutex->blocked_list= list_create();
	new_mutex->mutex_name = mutex_name;

	return new_mutex;
}

t_mutex * find_mutex_asigned_to_pcb(t_pcb * pcb, char * mutex_name){
	bool mutex_matches_name(void * p_void){
		t_mutex * p_mutex = (t_mutex*) p_void;
		return (!strcmp(p_mutex->mutex_name,mutex_name));
	};

	t_mutex * mutex = (t_mutex*) list_find(pcb->mutex_list,(void *)mutex_matches_name);
	
	return mutex;
}

bool mutex_is_taken (t_mutex * mutex){
	return (mutex->tcb) != NULL;
}

void asign_mutex(t_mutex * mutex, t_tcb* tcb){
	mutex->tcb= tcb;
	return;
}

bool mutex_is_taken_by_tcb (t_mutex * mutex, t_tcb* tcb){
	return mutex->tcb->tid == tcb->tid; 
}

void free_taken_mutexes(t_pcb* pcb,t_tcb*tcb_to_finish){// si me sobra tiempo implementar con map
	
	log_info(logger,"Liberando los mutex ocupados por (%d,%d)",tcb_to_finish->pid,tcb_to_finish->tid);
	int mutex_quantity = list_size(pcb->mutex_list);
	//int cantidad_de_tcb= list_size(pcb->tcb_list);
	//for(int k = 0; k<cantidad_de_tcb;k++){
	//	t_tcb * nashe = list_get(pcb->tcb_list,k);
	//	log_debug(logger,"(%d,%d) esta en estado %d",nashe->pid,nashe->tid,nashe->state);
	//}
	for(int i =0; i<mutex_quantity;i++){
		t_mutex * mutex= list_get(pcb->mutex_list,i);
		//int aux= list_size(mutex->blocked_list);
		//for(int j = 0; j<aux;j++){
		//	t_tcb * tcb_aux = list_get(mutex->blocked_list,j);
		//	log_debug(logger,"(%d,%d) esta bloqueado en %s en la posicion %d",tcb_aux->pid,tcb_aux->tid,mutex->mutex_name,j);
		//}
		//log_info(logger,"Mutex a revisar: %s",mutex->mutex_name);
		if(mutex->tcb && (mutex->tcb->tid == tcb_to_finish->tid)){
			//log_debug(logger,"(%d,%d) tiene tomado al mutex %s",tcb_to_finish->pid,tcb_to_finish->tid,mutex->mutex_name);
			mutex->tcb=NULL;
			if(!list_is_empty(mutex->blocked_list)){ //ojo con la relacion entre list_is_empty y la diferencia entre que la lista sea nula y que no tenga elementos
				t_tcb* next_tcb_to_unlock = list_remove(mutex->blocked_list,0);
				log_debug(logger,"(%d,%d) empieza a ocupar el mutex %s ahora",tcb_to_finish->pid,tcb_to_finish->tid,mutex->mutex_name);
				asign_mutex(mutex,next_tcb_to_unlock);
				change_block_type(next_tcb_to_unlock,INSTRUCTION);
				remove_tcb_from_state(next_tcb_to_unlock);
				add_tcb_to_state(next_tcb_to_unlock,READY);
			}
		}else{
			//if(mutex->tcb){
			//	log_debug(logger,"%s esta ocupado por (%d,%d), no por (%d,%d)",mutex->mutex_name,mutex->tcb->pid,mutex->tcb->tid,tcb_to_finish->pid,tcb_to_finish->tid);
			//}
		}
	}

};

t_mutex * find_mutex_by_blocked_thread(t_pcb * pcb, t_tcb*tcb){// devuelve un mutex si el tcb esta bloqueado dentro de su cola
	t_mutex* mutex= NULL;
	
	bool tcb_matches_tid(void * tcb_void){
		t_tcb * p_tcb = (t_tcb*) tcb_void;
		return p_tcb->tid == tcb->tid;
	};

	int mutex_quantity = list_size(pcb->mutex_list);

	bool not_found = true;

	for(int i =0; i<mutex_quantity && not_found;i++){
		t_mutex * aux_mutex= list_get(pcb->mutex_list,i);
		t_tcb * found_tcb = list_find(mutex->blocked_list,tcb_matches_tid);
		if(found_tcb){//lo encontro -> marco el bool para parar
			not_found=false;
			mutex = aux_mutex;
		}
	}

	return mutex;//-> sera null ni no lo encuentra// eso es teoricamente un error-> marcar con el debido logg
};

void destroy_mutex(t_mutex* mutex){
list_destroy(mutex->blocked_list);
//no libero el tcb que lo este ocupado-> no es responsabilidad de este hilo
free(mutex->mutex_name);
free(mutex);
}