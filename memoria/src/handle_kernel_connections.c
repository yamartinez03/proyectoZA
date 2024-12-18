#include "../include/handle_kernel_connections.h"

void procesar_kernel(void *void_socket){
	int *socket = (int*) void_socket;
	int socket_cliente = *socket;
	t_operation_code confirmacion_de_operacion = OK;
	t_operation_code operacion;
	
	if (recv(socket_cliente, &operacion, sizeof(operacion), MSG_WAITALL) != sizeof(operacion)) { 
			log_warning(logger_memoria, "ERROR en el envio del COD-OP por parte del CLIENTE"); 
			free(socket);
			return;
	}
	switch (operacion){ 
	case PROCESS_CREATE:

		t_nuevo_proceso* nuevo_proceso = recibir_process_create(socket_cliente);
		log_debug(logger_memoria,"Llega PROCESS_CREATE %d %s %d",nuevo_proceso->pid,nuevo_proceso->path,nuevo_proceso->tamanio);
		confirmacion_de_operacion = crear_proceso(nuevo_proceso->pid,nuevo_proceso->path,nuevo_proceso->tamanio);
		free(nuevo_proceso->path);
		free(nuevo_proceso);
		break;
	case FINISH_PROCESS:

		int pid_a_finalizar = recibir_pid_a_finalizar(socket_cliente);
		finalizar_proceso(pid_a_finalizar);

		break;
	case THREAD_CREATE:

		t_nuevo_hilo* nuevo_hilo = recibir_thread_create(socket_cliente);
		crear_hilo(nuevo_hilo->pid, nuevo_hilo->path, nuevo_hilo->tid);

		free(nuevo_hilo->path);
		free(nuevo_hilo); 
		break;
	case FINISH_THREAD:

		t_pid_tid* hilo_a_eliminar = recibir_pid_tid(socket_cliente);
		finalizar_hilo(hilo_a_eliminar->pid , hilo_a_eliminar->tid);

		free(hilo_a_eliminar);
		break;
	case DUMP_MEMORY:
		t_pid_tid* datos_proceso = recibir_pid_tid(socket_cliente);
		confirmacion_de_operacion = memory_dump(datos_proceso->pid , datos_proceso->tid);

		free(datos_proceso);
		break;
	case KERNEL:
		confirmacion_de_operacion = MEMORY;
		break;
	default:
		log_warning(logger_memoria,"CODIGO DE OPERACION NO RECONOCIDO");
		break;
	}
	log_debug(logger_memoria, "FIN DE PETICION FD:%d",socket_cliente);
	send(socket_cliente, &confirmacion_de_operacion, sizeof(t_operation_code), 0);
}

// CREACION DEL PROCESO

t_nuevo_proceso* recibir_process_create(int socket_cliente){
	t_list *paquete = recibir_paquete(socket_cliente);
	t_nuevo_proceso* nuevo_proceso = malloc(sizeof(t_nuevo_proceso));

	//EL ORDEN PUEDE CAMBIAR DEPENDIENDO DE COMO LO MANDE KERNEL. Estar atentos a cuando lo implementen
	int* pid = (int*)list_get(paquete,0);
	nuevo_proceso->pid = *pid;

	nuevo_proceso->path = strdup((char*)list_get(paquete, 1));

	uint32_t* tamanio = (uint32_t*) list_get(paquete,2);
	nuevo_proceso->tamanio = *tamanio;

	list_destroy_and_destroy_elements(paquete,free);
	return(nuevo_proceso);

}

//Se que es flashero que CREAR UN PROCESO devuelva un codigo de operacion, pero lo que devuelve es OK o ERROR si se creo o no. 
//Si hay duda preguntar, es por un problema de sincronizacion que encontre

t_operation_code crear_proceso(int pid, char* path, uint32_t tamanio){


	pthread_mutex_lock(&mutex_lista_particiones);

	t_operation_code confirmacion_espacio = buscar_espacio_memoria(tamanio);
	switch (confirmacion_espacio)
	{
	case OK:

		if(tamanio !=0){
			t_particion* particion_asignada = asignar_particion(pid,tamanio);
			pthread_mutex_unlock(&mutex_lista_particiones);		

			t_proceso* nuevo_proceso = malloc(sizeof(t_proceso));
			nuevo_proceso->pid = pid;
			nuevo_proceso->base = particion_asignada->base; 
			nuevo_proceso->limite = (particion_asignada->base) + (particion_asignada->tamanio); 
			nuevo_proceso->lista_tids = list_create();
			pthread_mutex_init(&(nuevo_proceso->mutex_lista_tids), NULL); 
			log_info(logger_memoria,"## Proceso Creado -  PID: %d - Tamaño: %d",pid, tamanio);
			
			push_con_mutex(lista_procesos, nuevo_proceso, &mutex_lista_procesos); 

			crear_hilo(pid, path, 0);
		}else{
			pthread_mutex_unlock(&mutex_lista_particiones);

			t_proceso* nuevo_proceso = malloc(sizeof(t_proceso));
			nuevo_proceso->pid = pid;
			nuevo_proceso->base = -1; 
			nuevo_proceso->limite = -1; 
			nuevo_proceso->lista_tids = list_create();
			pthread_mutex_init(&(nuevo_proceso->mutex_lista_tids), NULL); 
			log_info(logger_memoria,"## Proceso Creado -  PID: %d - Tamaño: %d",pid, tamanio);
			
			push_con_mutex(lista_procesos, nuevo_proceso, &mutex_lista_procesos); 

			crear_hilo(pid, path, 0);
		}
		


		return OK;

		break;
	case ERROR:
		log_debug(logger_memoria,"## NO HAY ESPACIO EN MEMORIA PARA CREAR (%d,%d)",pid, tamanio);
		pthread_mutex_unlock(&mutex_lista_particiones);

		return ERROR;
		break;
	default:
		log_debug(logger_memoria,"buscar_espacio_memoria FALLA y no devuelve un COD-OP correcto");

		pthread_mutex_unlock(&mutex_lista_particiones);
		return ERROR;
		break;
	}	
}

// CREACION DEL HILO

t_nuevo_hilo* recibir_thread_create(int socket_cliente){
	t_list* paquete = recibir_paquete(socket_cliente);
	t_nuevo_hilo* nuevo_hilo = malloc(sizeof(t_nuevo_hilo));


	int* pid = (int*)list_get(paquete,0);
	nuevo_hilo->pid = *pid;

	nuevo_hilo->path = strdup((char*)list_get(paquete, 1));

	int*tid = (int*)list_get(paquete,2);
	nuevo_hilo->tid = *tid;

	log_debug(logger_memoria, "THREAD CREATE. PID: %d, PATH: %s, TAMANIO: %d", *pid, nuevo_hilo->path, *tid);

	list_destroy_and_destroy_elements(paquete,free);
	return nuevo_hilo;

}

void crear_hilo(int pid, char* path, int tid){
	
	char* ruta_completa = string_from_format("%s%s", memoria_config->path_instrucciones,path); 

	log_debug(logger_memoria, "La RUTA COMPLETA es: %s", ruta_completa);

	t_list *lista_de_instrucciones = leer_pseudocodigo(ruta_completa);
	free(ruta_completa);
	

	t_hilo* nuevo_hilo = malloc(sizeof(t_hilo));
	nuevo_hilo->tid = tid;
	nuevo_hilo->registros_cpu = inicializar_registro_cpu();
	nuevo_hilo->instrucciones = lista_de_instrucciones;
	log_info(logger_memoria,"## Hilo Creado - (PID:TID) - (%d:%d)",pid, tid);

	t_proceso *proceso_padre = buscar_proceso(pid);//alto tiene que devolver si no lo encuentra
	
	log_debug(logger_memoria,"El proceso padre tiene el PID:%d", proceso_padre->pid);

	push_con_mutex(proceso_padre->lista_tids, nuevo_hilo, &(proceso_padre->mutex_lista_tids)); 
}

t_registros_cpu* inicializar_registro_cpu() {
	t_registros_cpu* nuevo_registros_cpu = malloc(sizeof(t_registros_cpu));

	nuevo_registros_cpu->AX = 0;
	nuevo_registros_cpu->BX = 0;
	nuevo_registros_cpu->CX = 0;
	nuevo_registros_cpu->DX = 0;
	nuevo_registros_cpu->EX = 0;
	nuevo_registros_cpu->FX = 0;
	nuevo_registros_cpu->GX = 0;
	nuevo_registros_cpu->HX = 0;
	nuevo_registros_cpu->PC = 0;

	return nuevo_registros_cpu;
}

//FINALIZACION DEL HILO

t_pid_tid* recibir_pid_tid(int socket_cliente){
	t_list* paquete = recibir_paquete(socket_cliente);
	t_pid_tid* hilo_a_eliminar = malloc(sizeof(t_pid_tid));

	int* pid = (int*)list_get(paquete,0);
	hilo_a_eliminar->pid = *pid;


	int* tid = (int*)list_get(paquete,1);
	hilo_a_eliminar->tid = *tid;


	log_debug(logger_memoria, "THREAD FINISH o MEMORY DUMP. PID: %d, TID: %d", *pid,*tid);

	list_destroy_and_destroy_elements(paquete,free);
	return hilo_a_eliminar;
}

void finalizar_hilo(int pid , int tid){
	t_proceso* proceso_del_hilo = buscar_proceso(pid);

	pthread_mutex_lock(&(proceso_del_hilo->mutex_lista_tids));

	int posicion_hilo = encontrar_posicion_hilo_en_lista(proceso_del_hilo->lista_tids,tid);
	list_remove_and_destroy_element(proceso_del_hilo->lista_tids, posicion_hilo, (void*)destructor_hilo); //TODO DESTRUCTOR_HILO
	
	pthread_mutex_unlock(&(proceso_del_hilo->mutex_lista_tids));

	log_info(logger_memoria,"## Hilo Destruido - (PID:TID) - (%d:%d)",pid,tid);
}

void destructor_hilo(t_hilo* hilo){
	free(hilo->registros_cpu);
	list_destroy_and_destroy_elements(hilo->instrucciones,free);
	free(hilo);
}



int encontrar_posicion_hilo_en_lista(t_list* lista_tids_proceso, int tid){

	for(int i = 0; i < list_size(lista_tids_proceso); i++){
		t_hilo* hilo_aux = list_get(lista_tids_proceso,i);

		if (hilo_aux -> tid == tid){
			return i;
		}
	}
	return (-1);
}

//FINALIZACION DEL PROCESO

int recibir_pid_a_finalizar(int socket_cliente){
	t_list *paquete = recibir_paquete(socket_cliente);

	int* PID = (int*)list_get(paquete,0);
	int pid = *PID;
	
	log_debug(logger_memoria, "PROCESS FINISH. PID: %d", pid);
	list_destroy_and_destroy_elements(paquete,free);
	return pid;
}

void finalizar_proceso(int pid){
	t_proceso* proceso_a_finalizar = buscar_proceso(pid);//si abajo tira seg fault es que no lo encontro

	log_info(logger_memoria, "## Proceso Destruido -  PID: %d - Tamaño: %d",pid, proceso_a_finalizar->limite - proceso_a_finalizar->base);
	//log_debug(logger_memoria, "NOTA PARA CUANDO DEBUG. EL LOG ANTERIOR APARECE ANTES DE DESTRUIR PARA NO TENER Q GUARDAR TODOS LOS DATOS EN VARIABLES");

	pthread_mutex_lock(&mutex_lista_particiones);
	//iterar_particiones();
	t_particion* particion_a_liberar = buscar_particion_por_base(proceso_a_finalizar->base); 
	if(particion_a_liberar){//Entra si el proceso no es de tamanio 0
		liberar_particion(particion_a_liberar); 
		//iterar_particiones();
	}
	pthread_mutex_unlock(&mutex_lista_particiones);

	pthread_mutex_lock(&mutex_lista_procesos);
	//iterar_procesos();
	list_remove_element(lista_procesos, proceso_a_finalizar);
	liberar_proceso(proceso_a_finalizar);
	//iterar_procesos();
	pthread_mutex_unlock(&mutex_lista_procesos); 

}

void iterar_procesos(){
	list_iterate(lista_procesos,mostrar_proceso);
}

void mostrar_proceso(void * ptr){
	t_proceso * proceso=(t_proceso*) ptr;
	log_debug(logger_memoria,"MOSTRANDO PROCESO -> PID: %d - BASE: %d - LIMITE: %d",proceso->pid,proceso->base,proceso->limite);
}

void liberar_proceso(t_proceso* proceso_a_finalizar){
	log_debug(logger_memoria, "Liberando estructuras utilizadas para el PROCESO");

	pthread_mutex_destroy(&(proceso_a_finalizar->mutex_lista_tids));

	if(proceso_a_finalizar-> lista_tids != NULL){
		list_destroy_and_destroy_elements(proceso_a_finalizar->lista_tids, (void*)destructor_hilo);
	}

	free(proceso_a_finalizar);
}

//MANEJO DE MEMORIA DE USUARIO

t_operation_code buscar_espacio_memoria(uint32_t tamanio) {

    // Definimos la función de búsqueda dentro del ámbito de la función principal
    bool buscar_vacio(void* particion) {
        t_particion* particion_aux = (t_particion*) particion;

        if (particion_aux->pid == (-1) && particion_aux->tamanio >= tamanio) {
            return true;
        } else {
            return false;
        }
    }

    // Usamos list_any_satisfy para buscar el espacio en memoria
    if (list_any_satisfy(lista_particiones, buscar_vacio)) { // No es necesario el cast
        log_debug(logger_memoria, "Hay espacio en memoria");
        return OK;
    } else {
        log_debug(logger_memoria, "NO hay espacio en memoria");
        return ERROR;
    }
}


t_particion* crear_particion(int pid, uint32_t base, uint32_t tamanio){
	
	t_particion* nueva_particion = malloc(sizeof(t_particion));
	nueva_particion->pid = pid;
	nueva_particion->base = base;
	nueva_particion->tamanio = tamanio;

	log_debug(logger_memoria,"Particion creada con PID: %d, BASE: %d, TAMANIO: %d", pid, base , tamanio);

	return nueva_particion;

}

bool comparador_base(void* particion1, void* particion2){
	t_particion* particion1Casteada = (t_particion*) particion1;
	t_particion* particion2Casteada = (t_particion*) particion2;

	return particion1Casteada->base < particion2Casteada->base;

}

t_particion* asignar_particion(int pid,uint32_t tamanio){
	t_particion* particion=NULL;
	
	if(strcmp(memoria_config->algoritmo_busqueda,"FIRST") == 0){
		particion = encontrar_particion_first(tamanio);
	}else if(strcmp(memoria_config->algoritmo_busqueda,"BEST") == 0){
		particion = encontrar_particion_best(tamanio);
	}else if(strcmp(memoria_config->algoritmo_busqueda, "WORST") == 0){
		particion = encontrar_particion_worst(tamanio);
	}else{
		log_debug(logger_memoria,"NO se esta reconociendo correctamente el algoritmo de busqueda");
	}

	if (strcmp(memoria_config->esquema,"DINAMICAS") == 0){
		t_particion* nueva_particion = crear_particion(pid, particion->base, tamanio);
		achicar_particion(nueva_particion,particion);
		list_add_sorted(lista_particiones,nueva_particion,(void*)comparador_base);

		//iterar_particiones();

		return nueva_particion;
	}else{
		particion->pid = pid;
		//iterar_particiones();
	}

	return particion;

}

void iterar_particiones(){
	list_iterate(lista_particiones,mostrar_particion);
};

void mostrar_particion(void* ptr) {
	t_particion* particion = (t_particion*) ptr;
	log_debug(logger_memoria,"MOSTRANDO PARTICION: BASE: %d - TAMAÑO: %d - PROCESO: %d ",particion->base , particion->tamanio, particion->pid);
};
void achicar_particion(t_particion* nueva_particion, t_particion* particion_a_modificar){
	particion_a_modificar->base = nueva_particion->base + nueva_particion ->tamanio;
	particion_a_modificar->tamanio = particion_a_modificar->tamanio - nueva_particion->tamanio;
	particion_a_modificar->pid = (-1); //Esto es al pedo pero para chequear 100% que quede como vacio

	if(particion_a_modificar->tamanio == 0){
		list_remove_element(lista_particiones,particion_a_modificar);
	}
}

int encontrar_posicion_particion_en_lista(t_particion *particion){
	for(int i = 0; i < list_size(lista_particiones); i++){
		t_particion* particion_aux = list_get(lista_particiones,i);

		if (particion_aux -> base == particion -> base){
			return i;
		}
	}
	return -1;
}

void liberar_particion(t_particion* particion_a_liberar){
	
	if(strcmp(memoria_config->esquema, "DINAMICAS") == 0){
		particion_a_liberar -> pid = -1;
		
		uint32_t limite = particion_a_liberar->base + particion_a_liberar->tamanio;
		if(limite != memoria_config->tam_memoria){
			t_particion* particion_libre_derecha = buscar_particion_libre(particion_a_liberar, true); //Leer el comentario de la funcion para entender porq el bool
			if(particion_libre_derecha){
				log_debug(logger_memoria, "Se encontro una PARTICION libre a la DERECHA");
				juntar_particiones(particion_a_liberar, particion_libre_derecha);
			}
		}
		
		if(particion_a_liberar->base != 0){
			t_particion* particion_libre_izquierda = buscar_particion_libre(particion_a_liberar, false); //Leer el comentario de la funcion para entender porq el bool

			if(particion_libre_izquierda){
				log_debug(logger_memoria, "Se encontro una PARTICION libre a la IZQUIERDA");
				juntar_particiones(particion_libre_izquierda, particion_a_liberar);
			}
		}
		
	}else{
		particion_a_liberar -> pid = -1;
		log_debug(logger_memoria, "Se marco la particion como libre");
	}
}

//Poner true si queres buscar a la derecha y false si queres buscar a la izquierda
t_particion* buscar_particion_libre(t_particion* particion, bool buscar_derecha) {

    int posicion_particion = encontrar_posicion_particion_en_lista(particion);
    
    // El (?) es como un if. Si buscar_derecha es TRUE hace posicion_particion + 1, si es FALSE hace posicion_particion - 1
	int posicion_a_buscar = buscar_derecha ? posicion_particion + 1 : posicion_particion - 1;
    t_particion* particion_adjacente = list_get(lista_particiones,posicion_a_buscar );
    
    if (particion_adjacente && particion_adjacente->pid == -1) {
        return particion_adjacente;
    } else {
        return NULL;
    }
}

void juntar_particiones(t_particion* particion_izquierda, t_particion* particion_derecha){


	log_debug(logger_memoria, "Base PI: %d, base PD: %d, tamanio PI: %d, tamanio PD: %d", particion_izquierda->base, particion_derecha->base, particion_izquierda->tamanio, particion_derecha->tamanio);
	particion_izquierda->tamanio += particion_derecha->tamanio;
	particion_izquierda->pid = -1; //NO va a hacer falta siempre ponerlo en -1 porq a veces ya va a venir libre pero lo dejo asi para las veces q no viene libre

	log_debug(logger_memoria, "Base PARTICION FINAL: %d, tamanio PARTICION FINAL: %d", particion_izquierda->base, particion_izquierda->tamanio);

	int posicion_particion_derecha = encontrar_posicion_particion_en_lista(particion_derecha);
	log_debug(logger_memoria, "Posicion de la particion a liberar: %d", posicion_particion_derecha);

	list_remove_and_destroy_element(lista_particiones, posicion_particion_derecha, free); 
	

}

//ALGORITMOS DE BUSQUEDA

t_particion* encontrar_particion_first(uint32_t tamanio){

	bool buscar_por_pid(void* particion){
	t_particion* particion_aux = (t_particion*) particion;

	if (particion_aux -> pid == (-1) && particion_aux->tamanio >= tamanio){
		return 1;
	}else{
		return 0;
	}
	};

	t_particion* particion = list_find(lista_particiones,(void*) buscar_por_pid);
	
	return particion;
}

t_particion* encontrar_particion_best(uint32_t tamanio) {

	bool cumple_condicion(void* particion){

		t_particion* particion_aux = (t_particion*) particion;
		return ((particion_aux->pid == (-1)) && (particion_aux->tamanio >= tamanio));
    
	};

    void* es_menor(void* particion1, void* particion2){

        t_particion* p1 = (t_particion*) particion1;
        t_particion* p2 = (t_particion*) particion2;

		if(p1->tamanio < p2->tamanio){
			return particion1;
		}else{
			return particion2;
		}

    
	};

    // Filtra las particiones vacias con tamanio suficiente
    t_list* particiones_validas = list_filter(lista_particiones,(void* ) cumple_condicion);

	if(!particiones_validas){
		log_debug(logger_memoria,"NO hay particiones validas");
	}
    // Obtener la partición con el menor tamaño
    t_particion* particion_best = list_get_minimum(particiones_validas,(void* ) es_menor);

    list_destroy(particiones_validas); 
    return particion_best;
}

t_particion* encontrar_particion_worst(uint32_t tamanio){

    bool cumple_condicion(void* particion) {

        t_particion* particion_aux = (t_particion*) particion;
        return (particion_aux->pid == -1 && particion_aux->tamanio >= tamanio);

    };

    void* es_mayor(void* particion1, void* particion2) {

        t_particion* p1 = (t_particion*) particion1;
        t_particion* p2 = (t_particion*) particion2;

		if(p1->tamanio > p2->tamanio){
			return particion1;
		}else{
			return particion2;
		}

    };


    t_list* particiones_validas = list_filter(lista_particiones, (void* )cumple_condicion);
    t_particion* particion_worst = list_get_maximum(particiones_validas,(void *) es_mayor);

    list_destroy(particiones_validas);
    return particion_worst;

}

//MEMORY DUMP

t_operation_code memory_dump(int pid , int tid){
	t_operation_code confirmacion_de_operacion;

	t_proceso* proceso = buscar_proceso(pid);
	uint32_t tamanio_asignado = (uint32_t)(proceso->limite - proceso->base); // No hace falta el casteo pero por las dudas

	if(tamanio_asignado != 0){

		fileSystem_connection = create_connection(logger_memoria, "FILESYSTEM", memoria_config->ip_fs, memoria_config->puerto_fs, TYPE_SOCKET_CLIENT);
		
		log_info(logger_memoria,"## Memory Dump solicitado - (PID:TID) - (%d:%d)",pid, tid);
	
		char* buffer = malloc(tamanio_asignado);

		pthread_mutex_lock(&mutex_memoria_usuario);
		memcpy(buffer, memoria_usuario + proceso->base, tamanio_asignado);
		pthread_mutex_unlock(&mutex_memoria_usuario);

		t_paquete* paquete = crear_paquete(DUMP_MEMORY);
		agregar_a_paquete(paquete, &pid, sizeof(int));
		agregar_a_paquete(paquete, &tid, sizeof(int));
		agregar_a_paquete(paquete, &tamanio_asignado , sizeof(uint32_t));
		agregar_a_paquete(paquete, buffer, tamanio_asignado);
		
		log_debug(logger_memoria, "Se mando a ejecutar MEMORY DUMP a FS");
		enviar_paquete(paquete, fileSystem_connection);
		recv(fileSystem_connection, &confirmacion_de_operacion, sizeof(t_operation_code), MSG_WAITALL); //VA A RECIBIR OK o ERROR
		eliminar_paquete(paquete);
		free(buffer);
		destroy_socket(fileSystem_connection);
	}else{
		confirmacion_de_operacion = ERROR;
	}
	

	return confirmacion_de_operacion;
}

