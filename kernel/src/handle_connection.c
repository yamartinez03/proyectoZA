#include <../include/handle_connection.h>


void planner_procedure(){
    //while(1);por ahora así para simular
    pthread_create(&thread_long_term,0,(void*)long_term_planner,NULL);
    pthread_detach(thread_long_term);
    
    pthread_create(&thread_short_term,0,(void*)short_term_planner,NULL);
    pthread_join(thread_short_term,NULL);
}

void memory_handshake(){
    t_operation_code operacion;
    memory_connection = create_connection(logger, "MEMORY", kernel_config->memory_ip, kernel_config->memory_port, TYPE_SOCKET_CLIENT);
	if (!send_handshake(memory_connection,KERNEL,MEMORY,logger))
        abort();
    recv(memory_connection,&operacion, sizeof(operacion),0);
    destroy_socket(memory_connection);
}