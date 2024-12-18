#include <../include/short_term.h>

void short_term_planner() {
	
	pthread_t io_handler_thread;
	pthread_create(&io_handler_thread,0,(void*) io_handler,NULL);
	pthread_detach(io_handler_thread); //NO JOIN

	pthread_t cpu_response_thread;
	pthread_create(&cpu_response_thread,0,(void*) cpu_response_handler,NULL);
	pthread_detach(cpu_response_thread); //NO JOIN
	
	//la idea es que no separamos en 3 funciones distintas a los 3 algoritmos de planificacion
	//si no que hacen todo igual en las cosas que coinciden, y despues en lo que difieren, se hace un switch segun el tipo de planificacion

	//PRIMER HILO - el planificador
	while(1){
		sem_wait(&plan_new_thread);
		t_tcb * tcb = NULL;
		do{
			sem_wait(&sem_counter_ready);
			tcb=NULL;
			log_debug(logger,"Hay al menos un hilo en ready, verifico si esta en exit o no");
			sem_wait(&sem_short_term);
			tcb = find_finished_tcb_in_ready();
			if(tcb){
				log_debug(logger,"Me quedo (%d,%d) en estado EXIT en la cola READY, lo quito y reintento planificar", tcb->pid,tcb->tid);
				sem_post(&(tcb->sem_finished_thread));
				tcb=NULL;
				log_debug(logger,"Hilo planificador LIBERA SEM_SHORT_TERM");
				sem_post(&sem_short_term);
			}else{ //entonces hay al menos un tcb en ready que no se ha finalizado
				log_debug(logger,"No hay hilos en ESTADO EXIT en cola READY");
				tcb = select_thread(); //selecciona un tcb de alguna cola segun el algoritmo
				if(!tcb){
					log_debug(logger,"Algo raro ocurrió: hay algun tcb en ready, no esta en exit pero aun asi no pudo encontrarlo: REINTENTO HASTA QUEDARME SIN INSTANCIAS DE SEM_COUNTER_READY");
					sem_post(&sem_short_term);
				}
				//NO HACE FALTA VERIFICAR EL ESTADO DE ESTE TCB
				//SI POR ALGUN MOTIVO NO ENCUENTRA UN TCB-> VUELVE A INTENTAR BLOQUEANDOSE
			}
		}while(!tcb);//esto es por si llega a fallar y no encuentra ninguno

		tcb->times_in_cpu++;
		remove_tcb_from_state(tcb);
		add_tcb_to_state(tcb,EXEC);
		send_tcb_to_cpu(tcb);
		sem_post(&sem_short_term);

		if(!strcmp(kernel_config->planning_algorithm,"CMN")){ //TODO-> ver bien como sería el string del algoritmo MULTICOLAS, no creo que sea "MULTICOLAS"
			log_debug(logger,"Comienzo a contar Quantum");
			usleep((kernel_config->quantum)*1000);// TO DO-> que tipo de dato es un quantum? por ahora lo dejo en int
			log_debug(logger,"Termine de contar Quantum");
			sem_wait(&sem_short_term);
			if(exec){ //SIEMPRE que quito a un tcb de exec, poner el puntero en NULL, POR EL MOTIVO QUE SEA, siempre quitar de estado antes de activar el semaforo de finalizacion
				log_debug(logger,"ENVIO INTERRUPCION DE QUANUTM");
				send_interrupt(tcb); // TO DO-> logica de 
			}else{
				log_debug(logger,"NO ENVIO INTERRUPCION DE QUANTUM");
			}
			sem_post(&sem_short_term);
		}
	}

}

t_tcb * find_finished_tcb_in_ready(){ //debe buscar en todas las colas ready posibles, algun tcb que este en estado exit, SOLO UNO
	t_tcb * tcb=NULL;

	bool tcb_is_finished(void * p_void){
		t_tcb * aux_tcb = (t_tcb*) p_void;
		return  (aux_tcb->state) == EXIT;
	};

	pthread_mutex_lock(&mutex_ready);
		if(!strcmp(kernel_config->planning_algorithm,"FIFO") || !strcmp(kernel_config->planning_algorithm,"PRIORIDADES")){
			tcb = list_find(ready,(void*)tcb_is_finished); //ready esta vacía-> devuelve null
			if(tcb){
				list_remove_element(ready,tcb);
			}
		}else{ //en este caso es MULTICOLAS
			int ready_sublists_quantity = list_size(ready);
			for(int i =0; i<ready_sublists_quantity && !tcb; i++){ //no importa la prioridad, debemos quitar los finalizados, de la cola que sea
				t_list* sublist = ((t_ready_priority_sublist *) (list_get(ready,i)))->sublist;
				tcb = list_find(sublist,(void*)tcb_is_finished);
				if(tcb){
					list_remove_element(sublist,tcb);
				}
			}
		}


	pthread_mutex_unlock(&mutex_ready);
	return tcb; //fijate que si no encuentra devuelve NULL
}

//OJO, los tcb necesitan mutex para poder accederse uno a la vez?

t_tcb * select_thread(){ //IMPORTANTE: devuelve NULL si no encuentra

	t_tcb * returned_tcb=NULL;

	pthread_mutex_lock(&mutex_ready);

	if(!strcmp(kernel_config->planning_algorithm,"FIFO")){
		
		if(!list_is_empty(ready)){
			returned_tcb=list_remove(ready,0); //obtengo el primero
		}
		
	}else if(!strcmp(kernel_config->planning_algorithm,"PRIORIDADES")){
		
		if(!list_is_empty(ready)){
			returned_tcb=list_remove(ready,0); //obtengo el primero
		}

	}else{//es multinivel
		
		t_ready_priority_sublist * priority_sublist = get_lowest_priority_queue(); //devuelve la primera NO vacia
		if(priority_sublist){
			returned_tcb= list_remove(priority_sublist->sublist,0);
		}
		
	}

	pthread_mutex_unlock(&mutex_ready);

	return returned_tcb;
}

t_ready_priority_sublist * get_lowest_priority_queue(){
	t_ready_priority_sublist * found_list = NULL;


	int cantidad_de_listas_creadas= list_size(ready);
	int prioridad_mas_baja_a_buscar = 0;
	bool repetir = true;
	int cantidad_de_listas_encontradas=0;

	do{
		t_ready_priority_sublist * tested_t_ready_priority_list= get_priority_list(prioridad_mas_baja_a_buscar);
		if(tested_t_ready_priority_list){
			repetir = list_is_empty(tested_t_ready_priority_list->sublist);
			cantidad_de_listas_encontradas++;
		}else{
			repetir=true;
		}
		prioridad_mas_baja_a_buscar++;
		if(!repetir){
			found_list = tested_t_ready_priority_list;
		}
	}while(repetir && cantidad_de_listas_encontradas<cantidad_de_listas_creadas);
	
	/*
	int sublist_quantity= list_size(ready);
	for(int i=0; !found_list && i<sublist_quantity;i++){
		t_ready_priority_sublist * aux = list_get(i);
		if(!list_is_empty(aux)){

		} 
	}
	*/
	/*
	int sublist_quantity= list_size(ready);
	for(int i=0; !found_list && i<sublist_quantity ; i++){ //ojo obtengo lista de menor prioridad pero puede estar vacia
		log_debug(logger,"busco sublista de prioridad %d",i);
		t_ready_priority_sublist * tested_t_ready_priority_list= get_priority_list(i);
		if(tested_t_ready_priority_list && !list_is_empty(tested_t_ready_priority_list->sublist)){ //la segunda validacion es por si encuentra una lista que esta vacia
			log_debug(logger,"sublista de prioridad %d encontrada",i);
			found_list = tested_t_ready_priority_list;
		}else{
			found_list = NULL;
			log_debug(logger,"sublista de prioridad %d  NO encontrada",i);
		}
	};
	*/

	return found_list;
}

t_ready_priority_sublist * get_priority_list(int priority){
	bool find_sublist_by_priority(void * p_void){
		t_ready_priority_sublist * tested = (t_ready_priority_sublist *) p_void;
		return priority == (tested->priority);
	};

	t_ready_priority_sublist * returned = list_find(ready,(void *)find_sublist_by_priority);

	return returned; //DEVUELVE NULL SI NO PUEDE ENCONTRAR LA LISTA
	
}





