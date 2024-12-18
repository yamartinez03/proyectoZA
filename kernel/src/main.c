#include <../include/main.h>

int main(int argc, char* argv[]) {
	
	path_process_0 = string_duplicate(argv[1]);//guardar path del proceso 0 en path_process_0, no hace falta mutex;
	size_process_0=(uint32_t) atoi(argv[2]);//guardar tamaño del proceso 0 en size_process_0, no hace falta mutex
	
	char * path_a_configuraciones = "./configuration/";

	char* config_prueba = string_from_format("%s%s", path_a_configuraciones,argv[3]); 
	
	printf("path_process_0 = '%s'\nsize_process_0 = %d\nconfig_path = %s\n",path_process_0,size_process_0,config_prueba);

	init_lists();
	init_mutex();
	init_semaphores();
	
    kernel_config = create_kernel_config(config_prueba);
	free(config_prueba);

	t_log_level log_level = log_level_from_string(kernel_config->log_level);

	logger = log_create("kernel.log", "KERNEL", 1,log_level);
	log_info(logger, "Kernel started");

   	cpu_dispatch_connection = create_connection(logger, "CPU DISPATCH", kernel_config->cpu_ip, kernel_config->cpu_dispatch_port, TYPE_SOCKET_CLIENT);
	if (!send_handshake(cpu_dispatch_connection,KERNEL,CPU,logger))//el cpu devuelva el t_operation_code_cpu y que loggee por pantalla
		abort();
	cpu_interrupt_connection = create_connection(logger, "CPU INTERRUPT", kernel_config->cpu_ip, kernel_config->cpu_interrupt_port, TYPE_SOCKET_CLIENT);
	init_connections();
    
	destroy_socket(cpu_dispatch_connection);
	destroy_socket(cpu_interrupt_connection);

    destroy_kernel_config(kernel_config);
    close_program(logger);

	return EXIT_SUCCESS;
};

void init_connections()
{
	pthread_t memory_handshake_thread; // lo creo aca porque es uno solo, esto verlo mas adelante
	pthread_create(&memory_handshake_thread, 0, (void *)memory_handshake, NULL);
	pthread_detach(memory_handshake_thread);

	pthread_create(&planner_thread, 0, (void *)planner_procedure, NULL); //hago join al hilo planificador para que no se me termine el main
	pthread_join(planner_thread,NULL);

};

