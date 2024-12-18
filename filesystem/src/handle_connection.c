#include "handle_connection.h"

void process_connection(void *void_socket){ 
    int *socket = (int*) void_socket;
	int socket_cliente = *socket;
    t_operation_code confirmacion_de_operacion;

    int operation;

    if (recv(socket_cliente, &operation, sizeof(operation), MSG_WAITALL) != sizeof(operation)) {
        log_warning(logger, "ERROR en el envio del COD-OP por parte del CLIENTE"); 
        free(socket);
        return;
    }
    switch (operation){
    //Ni hace falta el SWITCH ya que solo puede llegar una operaicon pero lo dejo por las dudas que memoria mande cualquier cosa
    case DUMP_MEMORY:
        t_datos_archivo* datos_archivo = recibir_memory_dump(socket_cliente);
        
        pthread_mutex_lock(&mutex_filesystem);
        confirmacion_de_operacion = ejecutar_memory_dump(datos_archivo);
        pthread_mutex_unlock(&mutex_filesystem);
        
        free(datos_archivo->buffer);
        free(datos_archivo);
        send(socket_cliente,&confirmacion_de_operacion,sizeof(t_operation_code),0);
        break;
    default:
        log_info(logger, "Se envio mal el codigo de operacion desde Memoria");
        break;
    }
    
}


t_datos_archivo* recibir_memory_dump(int socket_cliente){
    t_list *paquete = recibir_paquete(socket_cliente);
    t_datos_archivo* nuevo_archivo = malloc(sizeof(t_datos_archivo));

    int* pid = (int*)list_get(paquete,0);
    nuevo_archivo->pid = *pid;

    int* tid = (int*)list_get(paquete,1);
    nuevo_archivo->tid = *tid;

    int* tamanio = (int*)list_get(paquete,2);
    nuevo_archivo->tamanio = *tamanio;

    nuevo_archivo->buffer = strdup((char*)list_get(paquete,3));


    log_debug(logger, "Se recibio PID: %d, TID: %d, TAMANIO: %d y para escribir (esto no se si va a salir) %s", *pid, *tid, *tamanio, nuevo_archivo->buffer);

    list_destroy_and_destroy_elements(paquete,free);
    return(nuevo_archivo);
}

t_operation_code ejecutar_memory_dump(t_datos_archivo* datos_archivo){

    
    log_debug(logger, "Se empieza a ejecutar MEMORY DUMP");

    int bloques_necesarios = ceil((double)datos_archivo->tamanio / (double)filesystem_config->block_size) + 1;

    log_debug(logger, "La cantidad de bloques necesarios son: %d", bloques_necesarios);

    pthread_mutex_lock(&mutex_bitmap);

    t_operation_code confirmacion_espacio = buscar_espacio_bitmap(bloques_necesarios); 
    //Se que en esta funcion ya se podrian asignar los bloques, pero veo mucho mas ordenado y expresivo que sea separado


    switch (confirmacion_espacio)
    {
    case OK:
        log_debug(logger, "Hay espacio");

        char* nombre_archivo = generar_nombre_metadata(datos_archivo->pid,datos_archivo->tid);
        
        log_debug(logger, "El nombre del metadata sera: %s", nombre_archivo);

        t_list* indices_bloques_asignados = asignar_bloques_bitmap(bloques_necesarios, nombre_archivo);
        pthread_mutex_unlock(&mutex_bitmap);

        int indice_archivo_de_indices = get_int_from_list(indices_bloques_asignados,0);

        log_debug(logger, "El archivo de indices esta en el bloque: %d", indice_archivo_de_indices);

        crear_archivo_metadata(nombre_archivo, datos_archivo->tamanio, indice_archivo_de_indices);

        log_info(logger,"## Archivo Creado: %s- Tamaño: %d",nombre_archivo, datos_archivo->tamanio);

        escribir_archivo_bloques(datos_archivo, indices_bloques_asignados, nombre_archivo);

        log_info(logger, "## Fin de solicitud - Archivo: %s", nombre_archivo);

        list_destroy_and_destroy_elements(indices_bloques_asignados,free);
        free(nombre_archivo);
        return OK;
        break;
    
    case ERROR:

        log_info(logger,"NO hay espacio suficiente para escribir");
        pthread_mutex_unlock(&mutex_bitmap);

        return ERROR;
        break;
    default:
    	log_info(logger,"buscar_espacio_bitmap FALLA y no devuelve un COD-OP correcto");

		pthread_mutex_unlock(&mutex_bitmap);
		return ERROR;
        break;
    }
}

int cantidad_bloques_libres(){
    int cantidad_bloques_libres=0;

    for (off_t i = 0; i < filesystem_config->block_count; i++){
        if(!bitarray_test_bit(bitarray, i)){//bitarray todavia no esta creado
            cantidad_bloques_libres ++;
        }
    }

    return cantidad_bloques_libres;
}

t_operation_code buscar_espacio_bitmap(int bloques_necesarios){

    int bloques_libres = cantidad_bloques_libres();
    
    log_debug(logger, "Cantidad de bloques libres: %d", bloques_libres);
    if (bloques_libres >= bloques_necesarios){
        return OK;
    }else{
        return ERROR;
    }
    
}

t_list* asignar_bloques_bitmap(int bloques_necesarios, char* nombre_archivo){
    t_list* indices_de_bloques_encontrados = list_create(); //DESPUES HAY QUE LIBERARLA

    int bloques_encontrados=0;
    int i;
    for (i=0; i < filesystem_config->block_count && bloques_encontrados<bloques_necesarios; i++){
        if(!bitarray_test_bit(bitarray, i)){
            bitarray_set_bit(bitarray,i);
            uint32_t * aux = malloc(sizeof(uint32_t));
            *aux =i;
            list_add(indices_de_bloques_encontrados,aux);
            bloques_encontrados++;
            log_info(logger,"## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d", i, nombre_archivo, cantidad_bloques_libres());
        }
    } //sale del for si: reviso todos los bloques y no hay suficientes (esto NO PUEDE PASAR: logicamente se revisa antes de invocar a la funcino) |  encontro todos los bloques
    if((i == filesystem_config->block_count) && (bloques_encontrados < bloques_necesarios)){ //primer caso de salida (el invalido)
        abort(); //esto no puede pasar porque se verifica que haya suficientes bloques libres antes
    }
    return indices_de_bloques_encontrados;
}

//TOda la parte del tiempo y fecha hecho con CHAT GPT
char* generar_nombre_metadata(int pid, int tid) {
    time_t tiempo_actual;
    struct tm* tm_info;
    char timestamp[20];

    // Obtener el tiempo actual
    tiempo_actual = time(NULL);
    tm_info = localtime(&tiempo_actual);

    // Formatear el timestamp como YYYYMMDD-HHMMSS
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", tm_info);

    // Calcular el tamaño necesario para la cadena final
    size_t tamaño = snprintf(NULL, 0, "%d-%d-%s.dmp", pid, tid, timestamp) + 1;

    // Asignar memoria para el nombre del archivo
    char* nombre_archivo = malloc(tamaño);
    if (nombre_archivo == NULL) {
        perror("Error al asignar memoria");
        return NULL;
    }

    // Crear el nombre del archivo con snprintf
    snprintf(nombre_archivo, tamaño, "%d-%d-%s.dmp", pid, tid, timestamp);

    return nombre_archivo;  // Recordar liberar la memoria fuera de esta función
}



void escribir_archivo_bloques(t_datos_archivo*datos_archivo, t_list* lista_de_bloques_asignados, char* nombre_archivo){
    
    //datos generales
    int indice_bloque_indices = *((int*)list_get(lista_de_bloques_asignados,0));
    int base_bloque_de_indices = indice_bloque_indices * filesystem_config->block_size;
    
    //escritura de bloque de indices
    uint32_t * buffer_indices = convertir_lista_de_indices_a_buffer_de_indices(lista_de_bloques_asignados);
    escribir_parte_del_bloque(base_bloque_de_indices,buffer_indices,sizeof(buffer_indices));
    log_info(logger, "## Acceso Bloque - Archivo: %s - Tipo Bloque: INDICES - Bloque File System %d", nombre_archivo, indice_bloque_indices);
    free(buffer_indices);
    
    //escritura de bloques de datos
    int cantidad_de_accesos_a_bloques = list_size(lista_de_bloques_asignados);
    int resto_ultimo_bloque = datos_archivo->tamanio % filesystem_config->block_size;
    size_t desplazamiento_buffer = 0;
        //size_t desplazamiento_indices = 0;
    for (int i = 1; i < cantidad_de_accesos_a_bloques; i++) { //no contamos el bloque de indices
        uint32_t indice_bloque_a_escribir = (uint32_t)get_int_from_list(lista_de_bloques_asignados, i);
        int base_bloque = indice_bloque_a_escribir * filesystem_config->block_size;
        int bytes_a_escribir = 0;
        //escribir_parte_del_bloque(base_bloque_de_indices + desplazamiento_indices, &indice_bloque_a_escribir,sizeof(uint32_t));
        if(i != cantidad_de_accesos_a_bloques - 1){
            bytes_a_escribir = filesystem_config->block_size;
        }else{
            if(resto_ultimo_bloque == 0){
                bytes_a_escribir = filesystem_config->block_size;
            }else{
                bytes_a_escribir = resto_ultimo_bloque;
            }    
        }
        //se que esto ultimo es raro, pero asi se entiende mejor

        escribir_parte_del_bloque(base_bloque, (datos_archivo->buffer) + desplazamiento_buffer, bytes_a_escribir);
        log_info(logger, "## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System %u", nombre_archivo, indice_bloque_a_escribir);
        desplazamiento_buffer += bytes_a_escribir;
        //desplazamiento_indices += sizeof(uint32_t);
    }

}

void escribir_parte_del_bloque(int desplazamiento,void* buffer, int tamanio) {
    usleep(1000 * (filesystem_config->retardo_acceso_bloque));
    pthread_mutex_lock(&mutex_archivo_bloques);
    memcpy(buffer_bloques + desplazamiento, buffer, (size_t)tamanio);
    msync(buffer_bloques+desplazamiento,(size_t)tamanio,MS_SYNC);
    pthread_mutex_unlock(&mutex_archivo_bloques);
    
}

uint32_t* convertir_lista_de_indices_a_buffer_de_indices(t_list* lista) {
    int cantidad_de_bloques = list_size(lista);
    //el primero es siempre el bloque de indices, ese lo ignoramos
    uint32_t * buffer_indices = malloc(sizeof(uint32_t)*(cantidad_de_bloques-1));// resta 1 porque no cuenta el bloque de indices
    size_t desplazamiento_buffer_indices = 0;

    for (int i = 1; i < cantidad_de_bloques; i++) {
        uint32_t indice = (uint32_t) get_int_from_list(lista, i);  
        memcpy(buffer_indices+desplazamiento_buffer_indices,&indice,sizeof(uint32_t));
        desplazamiento_buffer_indices++; //necesitamos aumentarlo de uno en uno por la aritmetica de punteros
    }

    log_debug(logger,"Indice leido y escrito en buffer temporal de indices: %u",*(buffer_indices+desplazamiento_buffer_indices));
    
    return buffer_indices;
}

void crear_archivo_metadata(char *nombre, int tamanio, int nro_bloque_indice){
   
    char * completo = concatenar_a_mount_dir(nombre);
    
    FILE * archivo = fopen(completo,"w");
    fclose(archivo);

	t_config *nueva_metadata = config_create(completo);

	escribir_en_metadata(nueva_metadata, completo ,"SIZE",tamanio);
	escribir_en_metadata(nueva_metadata, completo ,"INDEX_BLOCK",nro_bloque_indice);
    log_debug(logger,"Valor seteado en metadata: %s",config_get_string_value(nueva_metadata,"SIZE"));
    log_debug(logger,"Valor seteado en metadata: %s",config_get_string_value(nueva_metadata,"INDEX_BLOCK"));
	return;
}

void escribir_en_metadata(t_config * metadata,char* path_al_meta,char * key, int valor){
	char * numero_en_string = string_itoa(valor);
	config_set_value(metadata, key,numero_en_string );
    config_save_in_file(metadata,path_al_meta);
    free(numero_en_string);
	return;
}

char* concatenar_a_mount_dir(char * resto){
    char * barrita_que_falta = "/";
    char * completo=string_from_format("%s%s%s",filesystem_config->mount_dir,barrita_que_falta,resto);
    log_debug(logger,"Unido a MONTAJE: %s ----> %s",resto,completo);
    return completo;
}
