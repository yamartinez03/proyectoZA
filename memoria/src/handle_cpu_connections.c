#include "../include/handle_cpu_connections.h"

void procesar_cpu(void *void_socket){ // Ya se que los nombres son mierda, pero sino tenia poner "args" y me parecia mucho menos expresivo 
	int *socket = (int*) void_socket;
	int socket_cliente = *socket;
	t_operation_code handshake = MEMORY;
	t_operation_code confirmacion_de_operacion = OK;
	t_operation_code operacion;
	t_paquete *paquete;
	while (socket_cliente != -1){
	log_debug(logger_memoria,"Esperando peticion por parte del CPU");
	if (recv(socket_cliente, &operacion, sizeof(operacion), MSG_WAITALL) != sizeof(operacion)) { 
			log_warning(logger_memoria, "ERROR en el envio del COD-OP por parte del CLIENTE"); 
			free(socket);
			return;
		}
		switch (operacion){
		case CPU:
			log_info(logger_memoria, "Se conecto: %s",translate_header(CPU));
			send(socket_cliente,&handshake,sizeof(t_operation_code),0);
			break;
		case OBTENER_CONTEXTO:
			paquete = recibir_obtener_contexto(socket_cliente);
			usleep(memoria_config->retardo * 1000);
			enviar_paquete(paquete,socket_cliente);
			eliminar_paquete(paquete);
			break;
		case ACTUALIZAR_CONTEXTO:
			recibir_actualizar_contexto(socket_cliente);
			usleep(memoria_config->retardo * 1000);
			send(socket_cliente,&confirmacion_de_operacion,sizeof(t_operation_code),0);
			break;
		case OBTENER_INSTRUCCION:
			paquete = recibir_obtener_instruccion(socket_cliente);
			usleep(memoria_config->retardo * 1000);
			enviar_paquete(paquete,socket_cliente);
			eliminar_paquete(paquete);
			break;
		case READ_MEM:
			paquete = recibir_READ_MEM(socket_cliente);
			usleep(memoria_config->retardo * 1000);
			enviar_paquete(paquete,socket_cliente);
			eliminar_paquete(paquete);
			break;
		case WRITE_MEM:
			recibir_WRITE_MEM(socket_cliente);
			usleep(memoria_config->retardo * 1000);
			send(socket_cliente,&confirmacion_de_operacion,sizeof(t_operation_code),0);
			break;
		default:
			break;
		}
		
	}
	
}

t_paquete *recibir_obtener_contexto(int socket_cliente){
	t_list *paquete_recibido = recibir_paquete(socket_cliente);

	int *pid = (int*)list_get(paquete_recibido,0);
	int *tid = (int*)list_get(paquete_recibido,1);

	log_info(logger_memoria,"## Contexto Solicitado - (%d:%d)",*pid,*tid);

	t_proceso *proceso = buscar_proceso(*pid);
	t_hilo *hilo;
	if(proceso){
		hilo = buscar_hilo(*pid,*tid);
	}

	t_paquete * paquete;
	if(proceso && hilo){
		paquete = crear_paquete(OBTENER_CONTEXTO);
		empaquetar_registros(paquete,hilo);
		empaquetar_base_y_limite(paquete, proceso);
	}else{
		int buffer = 0;
		paquete = crear_paquete(NOT_IN_MEMORY);
		agregar_a_paquete(paquete,&buffer,sizeof(int)); //esto lo agrego para que en ambos casos se pueda usar agregar a paquete, ya que creo que no puedo enviar_paquete con un paquete vacio
	}
	

	list_destroy_and_destroy_elements(paquete_recibido,free);
	return paquete;
}

void recibir_actualizar_contexto(int socket_cliente){
	t_list *paquete_recibido = recibir_paquete(socket_cliente);
	//DESEMPAQUETAR REGISTROS
	int *pid = (int*)list_get(paquete_recibido,0);
	int *tid = (int*)list_get(paquete_recibido,1);
	
	log_info(logger_memoria,"## Contexto Actualizado - (%d:%d)",*pid,*tid);

	t_hilo *hilo = buscar_hilo(*pid,*tid);
	
	if(hilo){
		log_debug(logger_memoria,"TID entontrado:%d",hilo->tid);
		//ACTUALIZAR REGISTROS
		(hilo->registros_cpu)->PC = get_uint32_t_from_list(paquete_recibido,2);
		(hilo->registros_cpu)->AX = get_uint32_t_from_list(paquete_recibido,3);
		(hilo->registros_cpu)->BX = get_uint32_t_from_list(paquete_recibido,4);
		(hilo->registros_cpu)->CX = get_uint32_t_from_list(paquete_recibido,5);
		(hilo->registros_cpu)->DX = get_uint32_t_from_list(paquete_recibido,6);
		(hilo->registros_cpu)->EX = get_uint32_t_from_list(paquete_recibido,7);
		(hilo->registros_cpu)->FX = get_uint32_t_from_list(paquete_recibido,8);
		(hilo->registros_cpu)->GX = get_uint32_t_from_list(paquete_recibido,9);
		(hilo->registros_cpu)->HX = get_uint32_t_from_list(paquete_recibido,10);
	}
	
	list_destroy_and_destroy_elements(paquete_recibido,free);

}

t_paquete *recibir_obtener_instruccion(int socket_cliente){
	t_list *paquete_recibido = recibir_paquete(socket_cliente);
	int *pid = (int*)list_get(paquete_recibido,0);
	int *tid = (int*)list_get(paquete_recibido,1);
	int *nro_instruccion = (int*)list_get(paquete_recibido,2);
	t_hilo *hilo = buscar_hilo(*pid,*tid);

	t_paquete * paquete=NULL;
	if(hilo){
		log_debug(logger_memoria,"Se pidio buscar la instruccion:%d del %d|%d", *nro_instruccion,*pid,*tid);
		char *sgte_instruccion = list_get(hilo->instrucciones,*nro_instruccion);

		log_info(logger_memoria,"## Obtener instrucción - (%d:%d) - Instrucción: %s",*pid,*tid,sgte_instruccion);

		paquete = crear_paquete(OBTENER_INSTRUCCION);
		empaquetar_instruccion(paquete,sgte_instruccion);
		list_destroy_and_destroy_elements(paquete_recibido,free);
	}else{
		int buffer = 0;
		paquete = crear_paquete(NOT_IN_MEMORY);
		agregar_a_paquete(paquete,&buffer,sizeof(int)); //esto lo agrego para que en ambos casos se pueda usar agregar a paquete, ya que creo que no puedo enviar_paquete con un paquete vacio
	}
	
	return paquete;
}


void empaquetar_instruccion(t_paquete * paquete,char* instruccion) {
    agregar_a_paquete(paquete, instruccion, strlen(instruccion) + 1); // +1 para incluir el '\0'
    return;
}


t_paquete *recibir_READ_MEM(int socket_cliente){
	t_list *paquete_recibido = recibir_paquete(socket_cliente);
	//uint8_t *direccion_recibida = (uint8_t*) list_get(paquete_recibido, 0);//lo pongo uint8_t porque apunta a 1 byte
	uint32_t direccion_recibida = get_uint32_t_from_list(paquete_recibido,0);
	int *pid = list_get(paquete_recibido,1);
	int *tid = list_get(paquete_recibido,2);
	//uint8_t *direccion = (uint8_t *) memoria_usuario +(uintptr_t) direccion_recibida;
	uint8_t *direccion = ((uint8_t *) memoria_usuario) + direccion_recibida;
	uint32_t registro;//sizeof(uint32_t) = 4

	log_info(logger_memoria,"Lectura - (%d:%d) - Dir. Física: %u - Tamaño: %zu",*pid,*tid, direccion_recibida, sizeof(uint32_t));

	pthread_mutex_lock(&mutex_memoria_usuario);
	memcpy(&registro, direccion, sizeof(uint32_t));
	pthread_mutex_unlock(&mutex_memoria_usuario);

	log_debug(logger_memoria,"(NO ES OBLIGATORIO) Leo: %d", registro);
	t_paquete * paquete = crear_paquete(READ_MEM);
	agregar_a_paquete(paquete,&registro,sizeof(uint32_t));

	list_destroy_and_destroy_elements(paquete_recibido,free);
	return paquete;
}

void recibir_WRITE_MEM(int socket_cliente){
	t_list *paquete_recibido = recibir_paquete(socket_cliente);
	//uint8_t *direccion_recibida = (uint8_t*) list_get(paquete_recibido, 0);//lo pongo uint8_t porque apunta a 1 byte
	uint32_t direccion_recibida = get_uint32_t_from_list(paquete_recibido,0);
	//uint8_t *direccion = (uint8_t *) memoria_usuario + (uintptr_t)direccion_recibida;
	uint8_t *direccion = ((uint8_t *) memoria_usuario) + direccion_recibida;
	uint32_t *registro = (uint32_t*) list_get(paquete_recibido, 1);
	int *pid = list_get(paquete_recibido,2);
	int *tid = list_get(paquete_recibido,3);

	log_info(logger_memoria,"Escritura - (%d:%d) - Dir. Física: %u - Tamaño: %zu",*pid,*tid, direccion_recibida, sizeof(uint32_t));

	log_debug(logger_memoria,"(NO ES OBLIGATORIO) Escribo: %d", *registro);
	pthread_mutex_lock(&mutex_memoria_usuario);
	memcpy(direccion,registro, sizeof(uint32_t));
	pthread_mutex_unlock(&mutex_memoria_usuario);

	uint32_t buffer =1;
	memcpy(&buffer,direccion,sizeof(uint32_t));
	log_debug(logger_memoria,"escrito: %d", buffer);

	list_destroy_and_destroy_elements(paquete_recibido,free);
}
