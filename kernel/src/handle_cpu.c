#include <../include/handle_cpu.h>

void cpu_response_handler (){
	//	!!!! mientras tanto, habra otro hilo que se encargara de esperar la respuesta del cpu en dispatch: separo esta logica en dos hilos porque -asi puedo contar el quantum paralelamente -habra motivos de desalojo que NO SIEMPRE impliquen replanificar, o sea, a veces la cpu nos envia un hilo y nosotros simplemente le devolvemos el mismo hilo -----> porque resultó ser una syscall NO DESALOJANTE / BLOQUEANTE, en resumen, no siempre despues de recibir la respuesta de la cpu, debemos REPLANIFICAR(seleccionar un proceso de alguna cola)
	//SEGUNDO HILO - EL QUE ESPERA LOS DESALOJOS
	while(1){
		sem_wait(&wait_cpu_response);
		log_debug(logger,"Esperando respuesta de CPU");
		t_cpu_return_data * cpu_return_data = receive_cpu_return(); // <---- es bloqueante !!! ojo que despues hay que liberar esta estructura-> si no memory leak
		sem_wait(&sem_short_term);

		switch(cpu_return_data->return_type){

			case SYSCALL:{ // OJO, EL CPU RETURN DATA YA TIENE LOS DATOS DUPLICADOS, NO HACE FALTA DUPLICAR ESTOS
				bool will_replan; // depende de cada syscall, habra replanificacion o no
				t_list * parameters_list = cpu_return_data->syscall_data->syscall_parameters;

				if(exec ){//si hubiese habido error por memory dump de ese proceso-> se hubiera quitado de exec al proceso
					t_tcb * running_tcb = exec; //lo pongo aca porque se usa en todos los casos
					t_pcb * running_pcb = find_pcb_from_tcb(running_tcb); // lo pongo aca porque se usa en todos los casos
					
					log_info(logger,"## (%d:%d) - Solicitó syscall: %s",running_tcb->pid,running_tcb->tid,syscall_code_to_string(cpu_return_data->syscall_data->syscall_code));
					//PRIMERO EL RECEIVE_CPU_RETURN DUPLICÓ LOS DATOS DE LA LISTA QUE MANDA CPU
					//AHORA TENEMOS QUE VOLVER A DUPLICAR LOS DATOS QUE SE GUARDAN EN HEAP, ASI LUEGO PODEMOS DESTRUIR TRANQUILO EL CPU_RETURN_TYPE
					switch(cpu_return_data->syscall_data->syscall_code){ 
					case PROCESS_CREATE:{
						char * main_thread_process_path = duplicated_string_from_list(parameters_list,0);
						uint32_t process_size = get_uint32_t_from_list(parameters_list,1);
						int main_thread_process_priority = get_int_from_list(parameters_list,2);
						
						log_debug(logger,"PATH %s - SIZE: %u - MAIN_THREAD_PRIORITY %d",main_thread_process_path,process_size,main_thread_process_priority);
						//debo crear y dejar en new: chequear todo lo que hay que hacer con largo plazo
						t_pcb * new_process = create_pcb(process_size,main_thread_process_path,main_thread_process_priority);//crear pcb 
    					push_con_mutex(new,new_process,&mutex_new);//agregar a new
    					sem_post(&sem_counter_new);//aumentar el semaforo en uno

						will_replan = false;
					break;
					}case PROCESS_EXIT:{
						if(running_pcb->state != EXIT){
							finish_process(running_pcb,running_tcb);
						}else{
							log_debug(logger,"OJO: EL PROCESO SIGUE EN EXEC, sin embargo el pcb esta en EXIT, algo pasó");
						}
					
						will_replan=true;
		
					break;
					}case THREAD_CREATE:{
						
						char* thread_path = duplicated_string_from_list(parameters_list,0); //debo duplicarlo porque debe hacersele free luego
						
						int thread_priority = get_int_from_list(parameters_list,1);

						//pthread_mutex_lock(&(running_pcb->pcb_access_mutex)); ME PARECE QUE NO HACE FALTA
						t_tcb* new_tcb = create_tcb(running_pcb, thread_path, thread_priority);
						//pthread_mutex_unlock(&(running_pcb->pcb_access_mutex)); ME PARECE QUE NO HACE FALTA
						//log_debug(logger,"CREO (%d,%d) PATH %s - THREAD_PRIORITY %d",new_tcb->pid,new_tcb->tid,thread_path,thread_priority);

						request_thread_creation_to_memory(new_tcb); // TO DO: debe ser bloqueante

						add_tcb_to_state(new_tcb,READY); //el post a ready esta adentro

						//justo en ESTE caso no es necesario el mutex de tcb_access_mutex, porque recien se crea entonces ningun otro hilo tiene todavía acceso
						
						will_replan = false;
						
					break;
					}case THREAD_JOIN:{

						int joinable_tcb_tid =  get_int_from_list(parameters_list,0);
						
						t_tcb * joinable_tcb = find_tcb(running_pcb,joinable_tcb_tid);
					
						//log_debug(logger,"(%d,%d) se intenta JOINEAR a (%d,%d): %d",running_tcb->pid,running_tcb->tid,running_tcb->pid,joinable_tcb_tid);
						if(joinable_tcb){ //lo encontro al tcb, pero ojo, todavía puede ser que este en exit, ojo con eso
							
							//pthread_mutex_lock(&(joinable_tcb->tcb_access_mutex));

							if(joinable_tcb->state == EXIT ){
								log_debug(logger,"(%d,%d) al que se intenta JOINEAR ya esta en EXIT",running_tcb->pid,joinable_tcb_tid);
								will_replan=false; //el hilo ya finalizo-> no me joineo
							}else{
								//pthread_mutex_lock(&(running_tcb->tcb_access_mutex));// NO HACE FALTA
								running_tcb->tid_of_tcb_joined_to = joinable_tcb->tid; 
								remove_tcb_from_state(running_tcb); //quito de exec
								list_add(joinable_tcb->joined_tcbs,running_tcb);
								change_block_type(running_tcb,THREAD_JOIN);
								add_tcb_to_state(running_tcb,BLOCK);

								log_info(logger,"## (%d:%d) - Bloqueado por: PTHREAD_JOIN",running_tcb->pid,running_tcb->tid);
								//pthread_mutex_unlock(&(running_tcb->tcb_access_mutex));// NO HACE FALTA
								will_replan=true;
							}

							//pthread_mutex_unlock(&(joinable_tcb->tcb_access_mutex));
							
						}else{ //todavia no existe ese tcb porque no se creo (no esta en la lista global de tcbs
							log_debug(logger,"Todavía no existe tal hilo");
							will_replan = false;
						}

					break;
					}case THREAD_CANCEL:{
						//change_tcb_state_from_global(parameters_list, EXIT );
						//send_tcb_to_memory(tcb, SYSCALL_THREAD_CANCEL); //HELP: Capaz deberiamos enviarle algun operationCode que conozca memoria

						int tid_to_finish =  get_int_from_list(parameters_list,0);

						log_debug(logger,"(%d,%d) es el hilo a intentar cancelar ",running_tcb->pid,tid_to_finish);

						t_tcb* tcb_to_finish = find_tcb(running_pcb,tid_to_finish);

						//pthread_mutex_lock(&(running_pcb->pcb_access_mutex)); //no hace falta 
						if(tcb_to_finish){ //lo encontró, ()
							//pthread_mutex_lock(&(tcb_to_finish->tcb_access_mutex));
							if(tcb_to_finish->state !=  EXIT ){ //si todavía no se finalizó el hilo
								log_debug(logger,"Existe el hilo y todavía no se finalizó");
								request_thread_release_to_memory(tcb_to_finish);
								free_taken_mutexes(running_pcb,tcb_to_finish);
								joined_threads_to_ready(tcb_to_finish);
								handle_sending_tcb_to_exit(running_pcb,tcb_to_finish);
							}else{
								log_debug(logger,"El hilo esta creado pero ya se finalizó");
							}//o sea, si es exit no hago nada
							//pthread_mutex_unlock(&(tcb_to_finish->tcb_access_mutex));
						}else{
							log_debug(logger,"Todavía no se creó tal hilo");
						}//si no lo encuentra es porque todavía no se creó
						//pthread_mutex_unlock(&(running_pcb->pcb_access_mutex)); //no hace falta 
						will_replan = false;

					break;
					}case THREAD_EXIT:{ //revisa si esta bien que no se use el mutex del tcb
						//pthread_mutex_lock(&(running_pcb->pcb_access_mutex));  NO ES NECESARIO
						//pthread_mutex_lock(&(running_tcb->tcb_access_mutex)); // NO ES NECESARIO pero lo dejo como formalidad
						log_info(logger,"## (%d:%d) Finaliza el hilo",running_tcb->pid,running_tcb->tid);
						request_thread_release_to_memory(running_tcb);
						free_taken_mutexes(running_pcb,running_tcb);
						joined_threads_to_ready(running_tcb);
						handle_sending_tcb_to_exit(running_pcb,running_tcb);
						//pthread_mutex_lock(&(running_tcb->tcb_access_mutex)); // NO ES NECESARIO pero lo dejo como formalidad
						//pthread_mutex_unlock(&(running_pcb->pcb_access_mutex)); NO ES NECESARIO

						will_replan=true;
					break;
					}case MUTEX_CREATE:{
						
						char * mutex_name =  duplicated_string_from_list(parameters_list,0);//es necesario duplicarlo porque debe guardarse 

						t_mutex* new_mutex = create_mutex(mutex_name);

						log_debug(logger,"Nuevo mutex %s creado por el hilo (%d,%d)",mutex_name,running_tcb->pid,running_tcb->tid);
						
						//pthread_mutex_lock(&(running_pcb->pcb_access_mutex));// NO HACE FALTA, lo dejo por formalidad

						list_add(running_pcb->mutex_list,new_mutex);

						//pthread_mutex_unlock(&(running_pcb->pcb_access_mutex)); // NO HACE FALTA, lo dejo por formalidad

						will_replan=false;

					break;
					}case MUTEX_LOCK:{
						char * mutex_name = get_string_from_list(parameters_list, 0); //no hace falta duplicarlo
						
						//pthread_mutex_lock(&(running_pcb->pcb_access_mutex)); //no hace falta
						//pthread_mutex_lock(&(running_tcb->tcb_access_mutex)); //no hace falta porque ningun otro hilo tendra acceso
							t_mutex * requested_mutex = find_mutex_asigned_to_pcb(running_pcb,mutex_name);
							//log_debug(logger,"Mutex buscado: %s",mutex_name);
							if(requested_mutex) {//si devuelve null
								//log_debug(logger,"%s encontrado",mutex_name);
								if(mutex_is_taken(requested_mutex)){ //se debe poner el puntero al tcb en null siempre que se libere un mutex ( ya sea con unlock o free taken)
									//log_debug(logger,"El mutex esta tomado por el hilo (%d,%d)",requested_mutex->tcb->pid,requested_mutex->tcb->tid);
									remove_tcb_from_state(running_tcb);
									list_add(requested_mutex->blocked_list,running_tcb);
									change_block_type(running_tcb,MUTEX_LOCK);
									add_tcb_to_state(running_tcb,BLOCK);
									log_info(logger,"## (%d:%d) - Bloqueado por: MUTEX",running_tcb->pid,running_tcb->tid);
									will_replan = true;
								}else{
									//log_debug(logger,"El mutex esta libre, se lo asigno a (%d,%d)",running_tcb->pid,running_tcb->tid);
									asign_mutex(requested_mutex,running_tcb);
									will_replan = false;
								}
							}else{ //no exite un mutex-> no dice que hacer en estos casos-> hago como que sigo normalmente la ejecucion
								//log_debug(logger,"Mutex %s no encontrado",mutex_name);
								will_replan = false;
							}
						//pthread_mutex_unlock(&(running_tcb->tcb_access_mutex)); //no hace falta porque ningun otro hilo tendra acceso
						//pthread_mutex_unlock(&(running_pcb->pcb_access_mutex)); //no hace falta

						
					break;
					}case MUTEX_UNLOCK:{
						char * mutex_name = get_string_from_list(parameters_list, 0);//no hace falta duplicar porque solo se usa para comparar
						
						//pthread_mutex_lock(&(running_pcb->pcb_access_mutex)); //NO ES NECESARIO
						//log_debug(logger,"Mutex a buscar: %s",mutex_name);

						t_mutex * requested_mutex = find_mutex_asigned_to_pcb(running_pcb,mutex_name);
						
						if(requested_mutex) {
							//log_debug(logger,"%s encontrado",mutex_name);
							if(mutex_is_taken_by_tcb(requested_mutex,running_tcb)){
								//log_debug(logger,"Mutex %s EXISTE y esta ocupado por el hilo",mutex_name);
								if(!list_is_empty(requested_mutex->blocked_list)){
									t_tcb * first_blocked_tcb = list_remove(requested_mutex->blocked_list,0);
									
									//pthread_mutex_lock(&(first_blocked_tcb->tcb_access_mutex)); //no es necesario
									//log_debug(logger,"(%d,%d) estaba primero esperando liberacion del mutex, lo ocupa y pasa a READY",first_blocked_tcb->pid,first_blocked_tcb->tid);

									requested_mutex->tcb=first_blocked_tcb;
									change_block_type(first_blocked_tcb,INSTRUCTION);
									remove_tcb_from_state(first_blocked_tcb);
									add_tcb_to_state(first_blocked_tcb,READY);

									//pthread_mutex_unlock(&(first_blocked_tcb->tcb_access_mutex)); //no es necesario

									will_replan = false;
								}else{
									//log_debug(logger,"Mutex %s no tiene hilos bloqueados esperando por su liberacion",mutex_name);
									requested_mutex->tcb=NULL;
									will_replan=false;
								}
							}else{
								//log_debug(logger,"Mutex %s EXISTE pero no esta ocupado por el hilo",mutex_name);
								will_replan= false;//aqui no se hace nada, lo dice la consigna
							}
						}else{ //no exite un mutex-> no dice que hacer en estos casos-> hago como que sigo normalmente la ejecucion
							//log_debug(logger,"Mutex %s no encontrado",mutex_name);
							will_replan = false;
						}
						
						//pthread_mutex_unlock(&(running_pcb->pcb_access_mutex)); //NO ES NECESARIO
						
					break;
					}case DUMP_MEMORY:{//lista de parametros vacia

						//debo poner al hilo en bloqueado
						//paralelamente hago la operacion a memoria -> paralelamente porque si no se detiene toda la planificacion
						//indico que se debe replanificar
						//pthread_mutex_lock(&(running_tcb->tcb_access_mutex)); //no es necesario 
						remove_tcb_from_state(running_tcb);
						change_block_type(running_tcb,DUMP_MEMORY);
						add_tcb_to_state(running_tcb,BLOCK);

						log_info(logger,"## (%d:%d) - Bloqueado por: DUMP_MEMORY",running_tcb->pid,running_tcb->tid);
						//pthread_mutex_unlock(&(running_tcb->tcb_access_mutex));  //no es necesario 

						pthread_t handle_memory_dump_thread;
						pthread_create(&handle_memory_dump_thread,0,(void*)handle_memory_dump,(void*)running_tcb);
						pthread_detach(handle_memory_dump_thread);

						will_replan = true;
						
					break;
					}case IO:{

						int miliseconds = get_int_from_list(parameters_list,0);

						log_debug(logger,"Milisegundos: %d",miliseconds);

						//pthread_mutex_lock(&(running_tcb->tcb_access_mutex));//no es necesario
						remove_tcb_from_state(running_tcb);
						change_block_type(running_tcb,IO);
						add_tcb_to_state(running_tcb,BLOCK);
						log_info(logger,"## (%d:%d) - Bloqueado por: IO",running_tcb->pid,running_tcb->tid);
						t_io_block_handler * new_io_block_handler = create_io_block_handler(running_tcb,miliseconds);
						push_con_mutex(io_block_list,new_io_block_handler,&mutex_io_block_list);
						sem_post(&sem_counter_io_block_list);
						//pthread_mutex_unlock(&(running_tcb->tcb_access_mutex));

						will_replan=true;

					break;
					}default:{
						break;
					}
				}
				}else{
					//se pidio finalizar el hilo por memory dump en todo este ciclo
					will_replan=true; //no hago la syscall
				}
				
				t_operation_code aux = cpu_return_data->syscall_data->syscall_code;
				cpu_return_data_destroy(cpu_return_data);
				if(will_replan){//TO DO: con cpu hecho, falta el temita del send y rcv
					log_debug(logger,"Se replanificará: la syscall %s produjo desalojo",syscall_code_to_string(aux));
					send(cpu_dispatch_connection,&will_replan,sizeof(bool),0);
					recv(cpu_dispatch_connection,&will_replan,sizeof(bool),MSG_WAITALL);
					sem_post(&sem_short_term);
					sem_post(&plan_new_thread);
				}else{
					log_debug(logger,"No se replanificará: la syscall %s NO produjo desalojo",syscall_code_to_string(aux));
					send(cpu_dispatch_connection,&will_replan,sizeof(bool),0);
					sem_post(&sem_short_term);
					sem_post(&wait_cpu_response);
				}
				
				
			break;
			}case INTERRUPTION:{ //ya sea desalojo de quantum 
			
				if(exec){ //exec es NULL si fallo el proceso por memory dump
					t_tcb * running_tcb = exec;
					remove_tcb_from_state(running_tcb);
					add_tcb_to_state(running_tcb,READY);
					log_info(logger,"## (%d:%d) - Desalojado por fin de Quantum",running_tcb->pid,running_tcb->tid);
				}else{
					log_debug(logger,"Hilo desalojado por QUANTUM pero su proceso fallo por MEMORY DUMP, entonces no vuelvo a meter a ready");
				}
					
					cpu_return_data_destroy(cpu_return_data);
					sem_post(&sem_short_term);
					sem_post(&plan_new_thread);
				
				break;

			}case SEGMENTATION_FAULT:{
			
				bool trash_value=0;
				log_debug(logger,"Error por Segmentation Fault recibido");

				if(exec){ //si se hizo finish process por dump, entonces SI O SI se quito al tcb de exec
					log_debug(logger,"Atiendo sin problemas");
					t_tcb * running_tcb = exec; 
					t_pcb * running_pcb = find_pcb_from_tcb(running_tcb); 
					finish_process(running_pcb,running_tcb);
				}else{
					log_debug(logger,"Pero antes el proceso falló por DUMP, entonces no vuelvo a finalizar el proceso");
				}

				send(cpu_dispatch_connection,&trash_value,sizeof(bool),0);
				recv(cpu_dispatch_connection,&trash_value,sizeof(bool),MSG_WAITALL);

				cpu_return_data_destroy(cpu_return_data);
				sem_post(&sem_short_term);
				sem_post(&plan_new_thread);

			break;
			}case NOT_IN_MEMORY:{ //es cuando cpu hizo operacion a memoria pero no encuentra el hilo-> desaloja (si no encuentra el hilo es porque)
				log_debug(logger,"CPU desalojo al hilo porque no lo encontró en memoria");
				bool trash_value=0; //lo llamo trash value porque cpu ya sabe que tendra que replanificar de todas formas
				send(cpu_dispatch_connection,&trash_value,sizeof(bool),0);
				recv(cpu_dispatch_connection,&trash_value,sizeof(bool),MSG_WAITALL);
				cpu_return_data_destroy(cpu_return_data);
				sem_post(&sem_short_term);
				sem_post(&plan_new_thread);
			}default:{
				cpu_return_data_destroy(cpu_return_data);
				log_debug(logger,"Invalid return type from CPU");
				sem_post(&sem_short_term);
				break;
			}
		}
		//¿que es lo bueno que tiene esto? en los casos en los que haya que replanificar, cambiamos de estado al tcb y hacemos sem_post(sem_short_term_planner) para seleccionar otro /en los casos en los que no hay que replanificar, primero devolvemos el tid y el pid al cpu y despues volvemos a esperar su respuesta (en este mismo hilo) haciendo sem_post(esperar_rta_del_dispatch)
	}
	
};

char * syscall_code_to_string (t_operation_code syscall_code){
	switch(syscall_code){
		case PROCESS_CREATE:
		return "PROCESS_CREATE";
		break;
		case PROCESS_EXIT:
		return "PROCESS_EXIT";
		break;
		case THREAD_CREATE:
		return "THREAD_CREATE";
		break;
		case THREAD_JOIN:
		return "THREAD_JOIN";
		break;
		case THREAD_CANCEL:
		return "THREAD_CANCEL";
		break;
		case THREAD_EXIT:
		return "THREAD_EXIT";
		break;
		case MUTEX_CREATE:
		return "MUTEX_CREATE";
		break;
		case MUTEX_LOCK:
		return "MUTEX_LOCK";
		break;
		case MUTEX_UNLOCK:
		return "MUTEX_UNLOCK";
		break;
		case DUMP_MEMORY:
		return "DUMP_MEMORY";
		break;
		case IO:
		return "IO";
		break;
		default:
		return "Invalid syscall code";
	}
}


t_cpu_return_data * receive_cpu_return(){

	t_operation_code return_type = recibir_operacion(cpu_dispatch_connection);
	t_list * list = recibir_paquete(cpu_dispatch_connection);

	#define PID_POSITION 0
	#define TID_POSITION 1
	#define SYSCALL_CODE_POSITION 2
	#define FIRST_SYSCALL_PARAMETER 3
	

	log_debug(logger,"Respuesta de CPU recibida");
	t_cpu_return_data * new_cpu_return_data = malloc(sizeof(t_cpu_return_data));
	new_cpu_return_data->return_type = return_type;
	new_cpu_return_data->pid= get_int_from_list(list,PID_POSITION);
	new_cpu_return_data->tid= get_int_from_list(list,TID_POSITION);
	new_cpu_return_data->syscall_data= malloc(sizeof(t_syscall_data));
	//new_cpu_return_data->syscall_data->syscall_code = 
	t_syscall_data* syscall_data = new_cpu_return_data->syscall_data;
	syscall_data->syscall_parameters=list_create();
	t_list * syscall_parameters = syscall_data->syscall_parameters;

	if(return_type == SYSCALL){ //en el caso de las syscalls debemos seguir sacando info de la lista
		t_operation_code syscall_code = (t_operation_code) get_int_from_list(list,SYSCALL_CODE_POSITION);
		syscall_data->syscall_code=syscall_code;
		switch(syscall_code){ //IMPORTANTE, no removemos los elementos de la lista ni usamos los mismos, obtenemos copias y luego borramos la lista original 
			case DUMP_MEMORY:{//DONE
				break;
			}case IO:{
				int * time = get_pointer_to_duplicated_int_from_list(list,FIRST_SYSCALL_PARAMETER);
				list_add(syscall_parameters,time);
				break;
			}case PROCESS_CREATE:{//DONE
				char * instructions_file = duplicated_string_from_list(list , FIRST_SYSCALL_PARAMETER);
				uint32_t * size = get_pointer_to_duplicated_uint32_t_from_list(list ,FIRST_SYSCALL_PARAMETER+1);
				int * thread_0_priority = get_pointer_to_duplicated_int_from_list(list ,FIRST_SYSCALL_PARAMETER+2);
				list_add(syscall_parameters,instructions_file);
				list_add(syscall_parameters,size);
				list_add(syscall_parameters,thread_0_priority);
				break;
			}case THREAD_CREATE:{//DONE
				char * instructions_file = duplicated_string_from_list(list ,FIRST_SYSCALL_PARAMETER);
				int * thread_priority = get_pointer_to_duplicated_int_from_list(list ,FIRST_SYSCALL_PARAMETER+1);
				list_add(syscall_parameters,instructions_file);
				list_add(syscall_parameters,thread_priority);
				break;
			}case THREAD_JOIN:{//DONE
				int * tid = get_pointer_to_duplicated_int_from_list(list , FIRST_SYSCALL_PARAMETER);
				list_add(syscall_parameters,tid);
				break;
			}case THREAD_CANCEL:{//DONE
				int * tid = get_pointer_to_duplicated_int_from_list(list ,FIRST_SYSCALL_PARAMETER);
				list_add(syscall_parameters,tid);
				break;
			}case MUTEX_CREATE:{
				char * mutex_name = duplicated_string_from_list(list ,FIRST_SYSCALL_PARAMETER);
				list_add(syscall_parameters,mutex_name);
				break;
			}case MUTEX_LOCK:{
				char * mutex_name = duplicated_string_from_list(list ,FIRST_SYSCALL_PARAMETER);
				list_add(syscall_parameters,mutex_name);
				break;
			}case MUTEX_UNLOCK:{
				char * mutex_name = duplicated_string_from_list(list ,FIRST_SYSCALL_PARAMETER);
				list_add(syscall_parameters,mutex_name);
				break;
			}case THREAD_EXIT:{
				//no hay mas parametros
				break;
			}case PROCESS_EXIT:{
				//no hay mas parametros
				break;
			}default:{
				abort();
				break;
			}
		}
	}else{
		syscall_data->syscall_code=INSTRUCTION; //porque ahora no recibimos codigo de syscall, ya se porque es segmentation fault o desalojo de quantums
		//IMPORTANTE: la lista de parametros queda CREADA pero vacia
	}
	
	list_destroy_and_destroy_elements(list,(void*)free); //destruimos la lista porque ya no la necesitamos (generamos copias de los elementos)
	
	return new_cpu_return_data;
};

void send_tcb_to_cpu(t_tcb * selected_thread){
	t_paquete * paquete = crear_paquete(THREAD);
	agregar_a_paquete(paquete,&(selected_thread->tid),sizeof(int));
	agregar_a_paquete(paquete,&(selected_thread->pid),sizeof(int));
	agregar_a_paquete(paquete,&(selected_thread->times_in_cpu),sizeof(int));
	log_debug(logger,"Selected thread - PID %d - TID %d",selected_thread->pid,selected_thread->tid);
	enviar_paquete(paquete,cpu_dispatch_connection);
	sem_post(&wait_cpu_response);//si, no esta muy bien que este aca, pero es para ponernos a escuchar apenas lo envie
	eliminar_paquete(paquete);
};

void send_interrupt(t_tcb * tcb){
	t_paquete * paquete = crear_paquete(INTERRUPTION);
	agregar_a_paquete(paquete,&(tcb->tid),sizeof(int));
	agregar_a_paquete(paquete,&(tcb->pid),sizeof(int));
	agregar_a_paquete(paquete,&(tcb->times_in_cpu),sizeof(int));
	enviar_paquete(paquete,cpu_interrupt_connection);
	eliminar_paquete(paquete);
}

void cpu_return_data_destroy(t_cpu_return_data * cpu_return_data){
	syscall_data_destroy(cpu_return_data->syscall_data);
	free(cpu_return_data);
	return;
};

void syscall_data_destroy(t_syscall_data * syscall_data){
	list_destroy_and_destroy_elements(syscall_data->syscall_parameters,free); //se deben poder eliminar todos los elementos de la lista con free
	free(syscall_data);
	return;
};

void tcb_and_pcb_release_handler(void * pcb_void){// TO DO -> agregar a modulo liberacion de memoria
	t_pcb* pcb = (t_pcb*) pcb_void;

	int tcb_quantity= list_size(pcb->tcb_list);

	for(int i=0; i<tcb_quantity;i++){
		t_tcb * tcb = (t_tcb*)list_get(pcb->tcb_list,i);
		//log_debug(logger,"Esperando a que (%d,%d) llegue a EXIT",tcb->pid,tcb->tid);
		sem_wait(&(tcb->sem_finished_thread)); //-> o sea, sirve para esperar a que todos los hilos del proceso ya no puedan ser accedidos por ningun otro hilo, para evitar seg faults
		destroy_tcb(tcb);
	};// para esto es importante que antes no se liberen ninguna de las estrcuturas de los pcbs y tcbs, ya que lo hara solo este hilo

	t_finalized_process * finalized_process = create_finalized_process(pcb);

	pthread_mutex_lock(&mutex_exit);
	list_add(exitt,finalized_process);
	pthread_mutex_unlock(&mutex_exit);

	destroy_pcb(pcb);
	//liberar cada tcb, quitandolos de exit y lista global ( pensando un destructor apropiado)
	//eliminar la cola exit
	//liberar el pcb, quitandolo de lista global (pensando un destructor apropiado)
	//agregar a cola exit, algun dato que sirva para guardar registro de sus pcbs y sus hilos ejecutados
	
}


void finish_process(t_pcb * pcb,t_tcb*tcb){

	log_info(logger,"## Finaliza el proceso %d",pcb->pid);
	
	//pthread_mutex_lock(&(PCB->pcb_access_mutex)); CREO QUE NO HACE FALTA
	change_pcb_state(pcb,EXIT); //importantisimo
	t_list * all_tcbs = find_all_tcbs_of_pcb(pcb);//conseguir todos los tcbs-> mira la commons LIST_FILTER puede servir
	
	int tcb_quantity = list_size(all_tcbs);//mas adelante fijate de hacer con map

	for(int i=0; i<tcb_quantity; i++){ // OJO, como vamos a revisar TODOS los tcbs (no solo los que no se finalizaron)
		t_tcb * tcb_to_finish = list_get(all_tcbs,i);
		//int pid= tcb_to_finish->pid;
		//int tid = tcb_to_finish->tid;
		//pthread_mutex_lock(&(tcb_to_finish->tcb_access_mutex));
		//problema-> puede haber algun hilo bloqueado esperando a volver de block que se le haya hecho cancel
		if(tcb_to_finish->state != EXIT){ //solo si no se pidio finalizar antes
			//free_taken_mutexes(pcb,tcb_to_finish); //creo que esto no hara falta SOLO EN FINISH PROCESS
			//joined_threads_to_ready(tcb_to_finish); //creo que esto no hara falta SOLO EN FINISH PROCESS
			handle_sending_tcb_to_exit(pcb,tcb_to_finish);	
		}
		//pthread_mutex_unlock(&(tcb_to_finish->tcb_access_mutex));
	}
	
	sem_wait(&sem_long_term);
	log_debug(logger,"Solicito finalizar PROCESO %d a MEMORIA",pcb->pid);
	request_process_release_to_memory(pcb);

	if(no_memory_for_new_process){ //recordar que si ese bool es true, es porque el credor de largo plazo esta bloqueado
		sem_post(&sem_retry_process_creation);
		no_memory_for_new_process=false;//como le hicimos post recien,ya no esta bloqueado
	};

	sem_post(&sem_long_term);

	//pthread_mutex_unlock(&(pcb->pcb_access_mutex)); CREO QUE NO HACE FALTA
	pthread_t tcb_and_pcb_release_thread;
	pthread_create(&tcb_and_pcb_release_thread,0,(void*)tcb_and_pcb_release_handler,pcb); //esto es para liberar la memoria usada 
	pthread_detach(tcb_and_pcb_release_thread); //NO DETACHEAR SI QUEREMOS ESPERAR A QUE CADA UNO HAGA LO QUE TENGA QUE HACER ANTES DE SEGUIR PLANIFICANDO

}
