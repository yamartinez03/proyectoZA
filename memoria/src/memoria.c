#include <../include/memoria.h>

int main(int argc, char* argv[]){

	char * path_a_configs = "./configuraciones/";
	char * config_prueba = string_from_format("%s%s", path_a_configs,argv[1]);
	printf("config_path = %s\n",config_prueba);

	memoria_config = create_memoria_config(config_prueba);
	free(config_prueba);

	t_log_level log_level = log_level_from_string(memoria_config->log_level);
	logger_memoria = log_create("memoria.log", "MEMORIA", 1,log_level);

	//logger_memoria = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_TRACE && LOG_LEVEL_INFO && LOG_LEVEL_WARNING);
	log_info(logger_memoria, "Memory started");

	inicializar_semaforos();
	
	inicializar_memoria_usuario();

	lista_procesos = list_create();

	procesar_conexiones();
	
	terminar_programa();
    return 0;
}

void procesar_conexiones(){

	memoria_servidor = create_connection(logger_memoria, "MEMORIA", NULL, memoria_config->puerto_esucha, TYPE_SOCKET_SERVER);
	
	int *cpu_socket = malloc(sizeof(int));
	*cpu_socket = waiting_client(memoria_servidor);

	pthread_t hilo_cpu;
	pthread_create(&hilo_cpu, NULL, (void*)procesar_cpu, (void*) cpu_socket);
	pthread_detach(hilo_cpu);

    while(escuchar_kernel());

}

int escuchar_kernel() { 

	int *kernel_socket = malloc(sizeof(int));
	*kernel_socket = waiting_client(memoria_servidor);
	log_info(logger_memoria, "## Kernel conectado - FD del socket: %d", *kernel_socket);

	if (kernel_socket >= 0){
		pthread_t hilo_kernel;
		pthread_create(&hilo_kernel, NULL, (void*)procesar_kernel, (void*) kernel_socket);
		pthread_detach(hilo_kernel);
		return 1;
	}

	return 0;
}

void inicializar_memoria_usuario(){

	log_debug(logger_memoria,"Inicializando memoria");
	memoria_usuario = malloc(memoria_config->tam_memoria);
	inicializar_listas_segun_esquema();

}

void inicializar_listas_segun_esquema(){

	//Si el esquema es particiones dinamicas la lista de particiones empieza con una particion vacia que ocupa TODO el espacio de memoria
	log_debug(logger_memoria,"Las particiones son:%s",memoria_config->esquema);

	if (strcmp(memoria_config->esquema,"DINAMICAS") == 0){ 
		log_debug(logger_memoria,"Se inicializan las estructuras para el manejo de PARTICIONES DINAMICAS");
		
		lista_particiones = list_create(); 

		t_particion* particion = crear_particion((-1) , 0 , memoria_config->tam_memoria);

		pthread_mutex_lock(&mutex_lista_particiones);
		list_add(lista_particiones, particion);
		pthread_mutex_unlock(&mutex_lista_particiones);		
	
	//Si el esquema es particiones fijas la lista de particiones ya arranca con todas las particiones hechas en base a lo que dice el archivo de configuracion
	}else{
		log_debug(logger_memoria,"Se inicializan las estructuras para el manejo de PARTICIONES FIJAS");
		uint32_t base_actual = 0;
		int cantidad_particiones = 0;

		for (int i = 0; memoria_config->particiones[i] != NULL; i++){
			cantidad_particiones ++;
		}
		
		log_debug(logger_memoria,"La cantidad de particiones a crear es: %d", cantidad_particiones);

		lista_particiones = list_create();

		for(int i = 0; i < cantidad_particiones; i++){

			t_particion* particion = crear_particion((-1), base_actual, atoi(memoria_config->particiones[i]));

			base_actual += particion->tamanio;

			pthread_mutex_lock(&mutex_lista_particiones);
			list_add(lista_particiones, particion);
			pthread_mutex_unlock(&mutex_lista_particiones);

		}			
	}
	
}

void terminar_programa(){
	free(memoria_usuario);
	destroy_memoria_config(memoria_config);
	
	list_destroy_and_destroy_elements(lista_particiones,free);
	list_destroy_and_destroy_elements(lista_procesos,free);//hacer un destructor de procesos.

	log_destroy(logger_memoria);


	close(fileSystem_connection);
	close(memoria_servidor);
}
