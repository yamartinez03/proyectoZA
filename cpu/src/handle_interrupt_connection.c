#include "handle_interrupt_connection.h"

void handle_interrupt_connection(){
    t_operation_code handshake = CPU;
    t_operation_code operacion;
    cliente_interrupt = waiting_client(cpu_interrupt_connection);//donde quedo el socket de escucha
    if(cliente_interrupt){
        log_debug(logger, "Se conecto un cliente por INTERRUPT");

        while (cliente_interrupt != -1){
            if(recv(cliente_interrupt,&operacion,sizeof(operacion),MSG_WAITALL) != sizeof(operacion)){
                log_warning(logger, "INVALID code operation from KERNEL - INTERRUPT"); 
            }

            switch (operacion)
            {
            case KERNEL: //ojo, no se si hay que hacer el handshake por interrupt tambien
                log_info(logger,"Se conecto KERNEL por INTERRUPT");
                send(cliente_interrupt, &handshake, sizeof(t_operation_code),0);
                break;
            case INTERRUPTION:
                t_list* list = recibir_paquete(cliente_interrupt);//coordinar con kernel el orden de como deberia quedar, pero en resumen, me quedan tres variables
                pthread_t interrupt_handler_thread;
                pthread_create(&interrupt_handler_thread,0,(void*)handle_interrupt,(void*)list);
                pthread_detach(interrupt_handler_thread);
                break;
            default:
                log_warning(logger, "Operación desconocida recibida: %d", operacion);
                break;
        }
        }
        log_info(logger, "Desconexión del cliente de INTERRUPT");
    }
    else {
        log_warning(logger, "Error al aceptar la conexión de INTERRUPT");
    }
}

void handle_interrupt(void * p_void){
    t_list * list = (t_list*) p_void;

    int tid=get_int_from_list(list,0);
    int pid=get_int_from_list(list,1);
    int time_in_cpu= get_int_from_list(list,2);
    list_destroy_and_destroy_elements(list,(void*)free);
    
    pthread_mutex_lock(&mutex_current_interruption);
    log_info(logger,"## Llega interrupción al puerto Interrupt");
    current_interruption.tid = tid;
    current_interruption.pid = pid;
    current_interruption.time_in_cpu = time_in_cpu;
    pthread_mutex_unlock(&mutex_current_interruption);

    return;
}
