#include <../include/handle_block.h>

t_io_block_handler * create_io_block_handler(t_tcb * tcb, int miliseconds){
	t_io_block_handler * new_io_block_handler = malloc(sizeof(t_io_block_handler));

	new_io_block_handler->tcb=tcb;
	new_io_block_handler->miliseconds=miliseconds;

	return new_io_block_handler;
}


void io_handler(){//FALTA OBTENER EL PCB
	while(1){
		sem_wait(&sem_counter_io_block_list);
		t_io_block_handler * waiting_thread = pop_con_mutex(io_block_list,&mutex_io_block_list);
		t_tcb * tcb = waiting_thread->tcb;
		log_info(logger,"IO_HANDLER atiende peticion de (%d,%d), espero %d milisegundos",tcb->pid,tcb->tid,waiting_thread->miliseconds);
		usleep(1000*(waiting_thread->miliseconds));
		
		sem_wait(&sem_short_term);

		if(tcb->state == EXIT){ //se lo pidio cancelar o se hizo finish process del proceso xd
			log_debug(logger,"Durante todo este tiempo, se pidio finalizar (%d,%d)",tcb->pid,tcb->tid);
			sem_post(&(tcb->sem_finished_thread));
			//si solo fue por cancel, entonces hasta que no se pida finalizar el proceso no le hara free
				//igualmente los joineados volvierona ready y se liberaron los mutex ocupados
			//si fue por finish proces, puede ser que lo libere o puede ser que no, depende de si quedan tcbs delegados, lo importante es que NO VUELVA A READY, quede solo en lista del pcb
		}else{
			change_block_type(tcb,INSTRUCTION);
			remove_tcb_from_state(tcb);//quitará de block // NO SE PUDO HABER MANDADO A EXIT
			add_tcb_to_state(tcb,READY);
			log_info(logger,"## (%d:%d) finalizó IO y pasa a READY",tcb->pid,tcb->tid);
		}

		free(waiting_thread);
		sem_post(&sem_short_term);
		
	}
}