#include <../include/tcb.h>

t_tcb * create_tcb(t_pcb * pcb,char * thread_path,int thread_priority){// siempre usar con el mutex de acceso al pcb correspondiente
	t_tcb * new_tcb = malloc(sizeof(t_tcb));

	//ahora agrego el nuevo tid a la lista de tid del pcb
	int * new_tid =malloc(sizeof(int)); 
	*new_tid=asign_tid_from_pcb(pcb); //no necesita mutex porque jamas dos hilos en un pcb se crean al mismo tiempo
	list_add(pcb->tid_list,new_tid);

	new_tcb->pid = pcb->pid ;
	new_tcb->tid=*new_tid;
	new_tcb->times_in_cpu=0;
	new_tcb->priority=thread_priority;
	new_tcb->path=thread_path;// que el path ya este en heap -> SE CUMPLE: tanto en largo plazo  (creacion de hilo 0), como en la creacion de hilos
	new_tcb->block_type = INSTRUCTION;
	new_tcb->joined_tcbs= list_create();
	new_tcb->pcb = pcb;
	new_tcb->tid_of_tcb_joined_to= (-1);
	pthread_mutex_init(&(new_tcb->tcb_access_mutex),NULL);
	log_info(logger,"## (%d:%d) Se crea el Hilo - Estado: READY", new_tcb->pid,new_tcb->tid);
	sem_init(&(new_tcb->sem_finished_thread),0,0);
	list_add(pcb->tcb_list,new_tcb);
	
	return new_tcb;
}

void change_state( t_tcb * tcb, t_state new_state){// utilizarla cuando ya use el mutex antes, pero si no lo lockee antes, puedo usar change_statewith_mutex
	tcb->state=new_state;
	return;
};

//add_to_state(state_list(tcb),tcb,state_mutex(tcb));
void add_tcb_to_state(t_tcb * tcb,t_state new_state){ 
	change_state(tcb,new_state);
	//t_list * new_state_list = state_list(tcb); 
	//pthread_mutex_t * new_state_mutex = state_mutex(tcb);

	switch(new_state){
		case NEW://no tiene sentido esto-> en new no van los tcbs
			log_debug(logger,"NO se puede agregar un tcb a new");
			abort();
			break;
		case READY:// NUNCA se hace push con mutex con ready
			
			log_info(logger,"Agrego (%d,%d) a READY",tcb->pid,tcb->tid);

			pthread_mutex_lock(&mutex_ready);
			
			if(!strcmp(kernel_config->planning_algorithm,"CMN")){
				t_ready_priority_sublist * ready_priority_sublist = get_priority_list(tcb->priority);
				if(ready_priority_sublist){ //quiere decir que encontro la lista porque ya habia sido creada
					//log_debug(logger,"SUBLISTA DE PRIORIDAD %d ENCONTRADA",ready_priority_sublist->priority);
					list_add(ready_priority_sublist->sublist,tcb);
				}else{ // es null es porque no encontro la lista, o sea, todavía no existe una cola con esa prioridad
					t_ready_priority_sublist * new_ready_priority_list = malloc(sizeof(t_ready_priority_sublist));
					new_ready_priority_list->priority = tcb->priority;
					new_ready_priority_list->sublist=list_create();

					list_add(new_ready_priority_list->sublist,tcb);
					list_add(ready,new_ready_priority_list);
					
					//log_debug(logger,"CREO SUBLISTA DE PRIORIDAD %d: HAY %d sublistas",new_ready_priority_list->priority,list_size(ready));
				}
			}else if(!strcmp(kernel_config->planning_algorithm,"PRIORIDADES")){
				list_add_sorted(ready,tcb,sort_by_priority); //inserto ordenadamente en ready
			}else{ //este es el caso de que es el algoritmo FIFO
				list_add(ready,tcb);//inserto ULTIMO en ready
			}

			pthread_mutex_unlock(&mutex_ready);

			sem_post(&(sem_counter_ready));
			break;			
		case EXEC:
			log_info(logger,"Agrego (%d,%d) a EXEC",tcb->pid,tcb->tid);
			pthread_mutex_lock(&mutex_exec);
			exec = tcb;
			pthread_mutex_unlock(&mutex_exec);
			break;
		case BLOCK: //NO HAGO NADA, YA HICE CHANGE STATE
			log_info(logger,"Agrego (%d,%d) a BLOCK",tcb->pid,tcb->tid);
			break;
		case EXIT://NO HAGO NADA, YA HICE CHANGE STATE
			log_info(logger,"Agrego (%d,%d) a EXIT",tcb->pid,tcb->tid);
			break;
		case NULL_STATE://NO HAGO NADA, YA HICE CHANGE STATE
			break;
	}

	
}

void remove_tcb_from_state(t_tcb* tcb){ //simplemente lo quitamos de su correspondiente estado
		
	
	switch(tcb->state){
		case NEW:
			log_debug(logger,"No puedo quitar (%d,%d) de NEW", tcb->pid,tcb->tid);
			abort();//no tiene sentido aca porque no hay tcbs en new
		break;
		case READY:
			log_info(logger,"Quito (%d,%d) de READY", tcb->pid,tcb->tid);
			//NO SE QUITA UN TCB DE READY a menos que se le haya hecho cancel o finish proces->SE LE CAMBIA EL ESTADO NADA MÁS, DELEGANDO LA ACTIVACION DEL SEMAFORO
		break;
		case EXEC:
			pthread_mutex_lock(&mutex_exec);
			log_info(logger,"Quito (%d,%d) de EXEC", tcb->pid,tcb->tid);
			exec = NULL;
			pthread_mutex_unlock(&mutex_exec);
		break;
		case BLOCK:
			log_info(logger,"Quito (%d,%d) de BLOCK", tcb->pid,tcb->tid);
			//no existe cola block
		break;
		case EXIT:
			log_debug(logger,"Intento quitar (%d,%d) de EXIT pero no hace nada porq ya esta en exit (no hago nada)", tcb->pid,tcb->tid);
			//no existe cola exit
		break;
		case NULL_STATE:
			log_debug(logger,"No puedo quitar (%d,%d) de NULL_STATE", tcb->pid,tcb->tid);;
			//no se quita NUNCA de null state
		break;
		default:
		break;
	};

	change_state(tcb,NULL_STATE); // si lo quitamos de su estado anterior, eso quiere decir que ahora no esta en ningun estado
	
};

/* 	importante
cada vez que jugamos con los estados y los tcb, usamos las funciones

remove_from_state para QUITAR de su estado segun el atributo
pero para que esta funcion FUNCIONE, entonces siempre que agregamos a un estado
hay que hacerlo con add to state, ya que coordina que siempre el atributo estado
apunte indique CORRECTAMENTE la lista a la que esta perteneciendo

además CUIDADO!!!!!!!!!!
estas funciones usan los mutex de las listas correspondientes
pero NO usan los mutex para el manejo de cada tcb para evitar 
que dos hilos gestionen un tcb al mismo tiempo

siempre despues de hacer remove_from_state, deberemos hacer add_to_state
porque no tiene sentido que un tcb no este en ningun estado

che,pero entonces
podria crearse una funcion que remueva un tcb de su estado y lo coloque en otro nuevo todo de una, o sea, que haga remove despues add

si es verdad
pero eso quitaria libertad para accionar en el orden que querramos, creo que es mejor separar las funciones asi y despues x cada caso invocamos

*/

t_tcb* find_tcb(t_pcb* pcb, int tid){
	
	bool tcb_matches_tid(void * p_void){
		t_tcb * p_tcb = (t_tcb*) p_void;
		return (p_tcb->tid==tid);
	};

	t_list * all_tcbs = find_all_tcbs_of_pcb(pcb);

	t_tcb * tcb = (t_tcb*) list_find(all_tcbs,(void*)tcb_matches_tid);
	
	return tcb; //ojo puede devolver NULL si no lo encuentra 
}

void change_block_type(t_tcb* tcb,t_operation_code new_block_type){
	tcb->block_type=new_block_type;
}


void joined_threads_to_ready(t_tcb * tcb_to_finish){ //si me sobra tiempo intentar aplicar con map->no es responsabilidad de este hilo eliminar la lista

	while(!list_is_empty(tcb_to_finish->joined_tcbs)){
		t_tcb * joined_tcb = list_remove(tcb_to_finish->joined_tcbs,0); //quito elemento pero fijate que no borro la lista, eso lo hace el destructor del tcb
		log_info(logger,"(%d,%d) estaba en la cola de joineados de (%d,%d), entonces lo pongo en ready",joined_tcb->pid,joined_tcb->tid,tcb_to_finish->pid,tcb_to_finish->tid);
		change_block_type(joined_tcb,INSTRUCTION);
		remove_tcb_from_state(joined_tcb);
		add_tcb_to_state(joined_tcb,READY);
	};//fijate en cuaderno POR QUE no hace falta bloquear al tcb accedido en este caso

};


void handle_sending_tcb_to_exit(t_pcb* pcb, t_tcb * tcb_to_finish){//usar siempre que vamos a finalizar un hilo
	switch(tcb_to_finish->state){ //no esta ni en new ni exit, ya nos encargamos antes
		case NEW:
			//no tiene sentido:error
		break;
		case READY: 
			//remove_tcb_from_state(tcb_to_finish); //pone en NULL STATE
			add_tcb_to_state(tcb_to_finish,EXIT); //PONGO ESTADO EN EXIT
			//fijate que NO ACTIVO EL SEMAFORO
		break;
		case EXEC:
			remove_tcb_from_state(tcb_to_finish); //PONE EN NULL STATE Y EXEC=NULL
			add_tcb_to_state(tcb_to_finish,EXIT); //PONE EN ESTADO EXIT
			sem_post(&(tcb_to_finish->sem_finished_thread));
		break;
		case BLOCK:{

			switch (tcb_to_finish->block_type)
			{
			case IO:
				remove_tcb_from_state(tcb_to_finish); //PONE EN NULL STATE
				add_tcb_to_state(tcb_to_finish,EXIT); // PONE ESTADO EN EXIT 
				//fijate que NO ACTIVO EL SEMAFORO: SIGNIFICA QUE TIENE RESPONSABILIDAD DELEGADA
				break;
			case DUMP_MEMORY:
				remove_tcb_from_state(tcb_to_finish); //PONE EN NULL STATE
				add_tcb_to_state(tcb_to_finish,EXIT); // PONE ESTADO EN EXIT
				//fijate que NO ACTIVO EL SEMAFORO: SIGNIFICA QUE TIENE RESPONSABILIDAD DELEGADA
				break;
			case MUTEX_LOCK: /// debo quitar el tcb de la cola de bloqueados del mutex en el que este bloqueado
				t_mutex * mutex = find_mutex_by_blocked_thread(pcb,tcb_to_finish);// NO PUEDE DEVOLVER NULL teoricamente
				list_remove_element(mutex->blocked_list,tcb_to_finish);// TO DO arreglas mas adelante que pasa si este devuelve FALSE
				change_block_type(tcb_to_finish,INSTRUCTION);//cambio estado
				remove_tcb_from_state(tcb_to_finish);
				add_tcb_to_state(tcb_to_finish,EXIT);
				sem_post(&(tcb_to_finish->sem_finished_thread)); //todavía no se puso a finalizar el proceso entonces no hay problema
				break;
			case THREAD_JOIN:
				t_tcb* tcb_joined_to = find_tcb(pcb,tcb_to_finish->tid_of_tcb_joined_to);
				tcb_to_finish->tid_of_tcb_joined_to=(-1); // porque no nos interesa mas
				//pthread_mutex_lock(&(tcb_joined_to->tcb_access_mutex)); // me parece que no es necesario este mutex igual, ya que un hilo paralelo solo pone el tcb en ready (por que nunca podria mandarlo a exit lo dice en mi cuaderno), lo dejo por las dudas, cualquier cosa lo saco
				list_remove_element(tcb_joined_to->joined_tcbs,tcb_to_finish); // TO DO ver que pasa si no lo encuentra, igual teoricamente siempre deberia encontrarlo
				/*si handle_send_to_exit fue invocado por FINISH PROCESS-> no me importa si el hilo fue previamente finalizado o no*/
				//pthread_mutex_unlock(&(tcb_joined_to->tcb_access_mutex));
				change_block_type(tcb_to_finish,INSTRUCTION);
				remove_tcb_from_state(tcb_to_finish);
				add_tcb_to_state(tcb_to_finish,EXIT);
				sem_post(&(tcb_to_finish->sem_finished_thread)); //todavía no se puso a finalizar el proceso entonces no hay problema
				break;
			default:
				log_debug(logger,"Codigo de tipo de bloqueo desconocido: %d",tcb_to_finish->block_type);
				abort();
				break;
			}
		}break;	
		case EXIT:
			//no hago nada, NI SIQUIERA ACTIVAR EL SEMAFORO igual no puede pasar de invocarse handle send to exit si ya estaba previamente en exit
			break;
		case NULL_STATE:
			add_tcb_to_state(tcb_to_finish,EXIT);
			sem_post(&(tcb_to_finish->sem_finished_thread));
		break;
	};

	return;
};

t_list * find_all_tcbs_of_pcb(t_pcb * pcb){
	return pcb->tcb_list;
}

void destroy_tcb(t_tcb*tcb){
	free(tcb->path);
	list_destroy(tcb->joined_tcbs);//no es responsabiliadad de este hilo finalizar los hilos que esten joineados
	pthread_mutex_destroy(&(tcb->tcb_access_mutex));
	sem_destroy(&(tcb->sem_finished_thread));
	free(tcb);
};
