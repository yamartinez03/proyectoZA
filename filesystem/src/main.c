#include "main.h"

//IMPORTANTE

// OJO, el MAKEFILE de forma predeterminada entiende que los .h estan en la carpeta src con los .c, si lo van a cambiar, creando una carpeta src, cambien el makefile



int main(int argc, char* argv[]) {
	saludar("filesystem");

	char * path_a_configs = "./configuraciones/";
	char * config_prueba = string_from_format("%s%s", path_a_configs,argv[1]);
	printf("config_path = %s\n",config_prueba);

	filesystem_config = create_filesystem_config(config_prueba);
	free(config_prueba);

	t_log_level log_level = log_level_from_string(filesystem_config->log_level);
	logger = log_create("filesystem.log", "FILESYSTEM", 1,log_level);
	//logger = log_create("filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_TRACE && LOG_LEVEL_INFO && LOG_LEVEL_DEBUG); // 	log_create crea el archivo log si no existe?
	log_info(logger, "Filesystem started");


	inicializar_archivos();
 	inicializar_semaforos();

    filesystem_listening_socket = create_connection(logger, "FILESYSTEM", NULL, filesystem_config->puerto_esucha, TYPE_SOCKET_SERVER);

    while(server_escuchar()){};//falta server_escuchar
    
	terminar_programa();
    return 0;

}
//filesystem_listening_socket es el socket de escucha mediante el cual estamos escuchando (con accept) nuevas conexiones, se usa para, por cada cliente, obtener otro socket nuevo, el bidireccional

int server_escuchar() { 

    int * bidirectional_connection = malloc(sizeof(int)); //bidirecitional connection NO PUEDE SER una variable global

	if ((*bidirectional_connection = waiting_client(filesystem_listening_socket))) {
		log_debug(logger,"Se conecto MEMORIA");
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*)process_connection, (void*) bidirectional_connection);
		pthread_detach(hilo);
		return 1;
	}


	return 0;
}

void inicializar_archivos(){
	abrir_bitmap();
    abrir_archivo_bloques();
}

void abrir_bitmap(){
	int fd;
	tamanio_bitmap = ceil((double)filesystem_config->block_count/8);
	bool fue_creado;
	if(access(filesystem_config->path_bitmap, F_OK) == (-1)){
		log_debug(logger, "No existe el archivo bitmap, asi que lo creamos.");
		fd = open(filesystem_config->path_bitmap, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if(fd == (-1)){
			log_error(logger, "No se pudo abrir o crear el archivo bitmap: %s", strerror(errno));
			abort();
		}
		if(ftruncate(fd, tamanio_bitmap) == (-1)){
			 log_error(logger, "No se pudo establecer el tamaño del archivo bitmap: %s", strerror(errno));
			close(fd);
			abort();
		}
		fue_creado=true;
	}else{
		fd = open(filesystem_config->path_bitmap, O_RDWR);
		if(fd == (-1)){
			log_error(logger, "No se pudo abrir o crear el archivo bitmap: %s", strerror(errno));
			abort();
		}
		fue_creado=false;
	}

	buffer_bitmap = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buffer_bitmap == MAP_FAILED) {
		log_error(logger, "Error al mapear el bitmap.");
		close(fd);
		abort();
	}
	bitarray = bitarray_create_with_mode(buffer_bitmap,tamanio_bitmap,MSB_FIRST);
	log_debug(logger,"El bitmap fue creado");
	close(fd);
	
	if(fue_creado){
		setear_bitmap();
		log_debug(logger,"Bitmap seteado");
	}else{
		log_debug(logger,"No se setea el bitmap, porque ya fue utilizado en otra ejecucion");
	}
	
	
}

void setear_bitmap(){
	for(off_t i=0;i<filesystem_config->block_count;i++){
		bitarray_clean_bit(bitarray,i);
	}	
}

void abrir_archivo_bloques(){
	int fd;
	tamanio_filesystem = filesystem_config->block_size*filesystem_config->block_count;

	if(access(filesystem_config->path_bloques, F_OK) == (-1)){
		log_debug(logger, "No existe el archivo de bloques, asi que lo creamos.");
		fd = open(filesystem_config->path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if(fd == (-1)){
			log_error(logger, "No se pudo crear el archivo de bloques.");
			close(fd);
			abort();
		}
		if(ftruncate(fd, tamanio_filesystem) == (-1)){
			log_error(logger, "No se pudo establecer el tamaño del archivo de bloques %s", strerror(errno));
			close(fd);
			abort();
		}
	}else{
		fd = open(filesystem_config->path_bloques, O_RDWR);
		if(fd == (-1)){
			log_error(logger, "No se pudo abrir el archivo de bloques.");
			abort();
		}
	}

	buffer_bloques = (char*)mmap(NULL, tamanio_filesystem, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(buffer_bloques == MAP_FAILED){
		log_error(logger, "Error al mapear el archivo de bloques.");
		close(fd);
		abort();
	}
	log_debug(logger,"El archivo de bloques fue creado.");
	close(fd);

}

void terminar_programa(){
    destroy_filesystem_config(filesystem_config);
	log_destroy(logger);

	free(bitarray);
	free(buffer_bloques);
}
