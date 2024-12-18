#include "cpu.h"

pthread_t thread_dispatch, thread_interrupt, thread_cpu_memory;


int main(int argc, char* argv[]) {
  cpu_config = create_cpu_config("configuration/cpu.config");

  t_log_level log_level = log_level_from_string(cpu_config->log_level);
	logger = log_create("cpu.log", "CPU", 1,log_level);
  //logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_TRACE && LOG_LEVEL_DEBUG);
  log_info(logger, "CPU started");
  init_semaphores();
  init_connections();
  close_program(logger);
  return EXIT_SUCCESS;
}

void init_connections(){
  memory_connection = create_connection(logger, "CPU", cpu_config->memory_ip, cpu_config->memory_port, TYPE_SOCKET_CLIENT);
  if (!send_handshake(memory_connection,CPU,MEMORY,logger))
  {
    abort();
  }
  
  cpu_dispatch_connection = create_connection(logger,"CPU DISPATCH",NULL,cpu_config->listen_dispatch_port,TYPE_SOCKET_SERVER);
  cpu_interrupt_connection = create_connection(logger,"CPU INTERUPT",NULL,cpu_config->listen_interrupt_port,TYPE_SOCKET_SERVER);

  pthread_create(&thread_interrupt,NULL, (void *)handle_interrupt_connection, NULL);
  pthread_detach(thread_interrupt);

  pthread_create(&thread_dispatch,NULL,(void*) handle_dispatch_connection,NULL);
  pthread_join(thread_dispatch,NULL);

  destroy_socket(memory_connection);

}

void init_semaphores(){
  sem_init(&sem_receive_new_thread,0,2);//en teoria dos, uno para el handshake y otro para el primer thread
  pthread_mutex_init(&mutex_current_interruption,NULL);
}


