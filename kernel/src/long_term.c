#include <../include/long_term.h>

void long_term_planner(){

    //tener en cuenta que a los procesos los crean los demás procesos, entonces cuando uno usa una syscall para crear un proceso
    //se crea el pcb y se agrega a esta cola, por lo tanto este hilo debe estar constantemente esperando para iniciar otros hilos
    //IMPORTANTE: en la cola new tendremos los PCBs nuevos, pero todavía no hay tcbs de hilos creados

    /*
    Vos cuando creas un proceso, le pedis a la memoria reservar espacio para ese PROCESO, MEMORIA ASUME como que el path que le pasas es para el hilo tcb
    y tiene sentido, es un proceso que recien se creo, obviamente que el path de pseudocodigo sera el de su tcb 0, entonces una vez que nos dice que ya reservo espacio
    listo, no tengo que pedirle nada más a la memoria, simplemente creo la estructura de TCB Y lo agrego a ready, pero no pido crear ningun hilo a la memoria.
    */

    t_pcb * first_process = create_pcb(size_process_0,path_process_0,0);//crear pcb 0
    push_con_mutex(new,first_process,&mutex_new);//agregar el p0 a cola new
    sem_post(&sem_counter_new);//aumentar el semaforo en uno

    log_debug(logger,"Primer PCB agregado a NEW");
    
    while(1){//por cada syscall de creacion de proceso, agregar el pcb a la cola new y aumentar el semaforo contador, para desbloquear este while
    sem_wait(&sem_counter_new);//usar el sem para que me habiliten a hacer GET (para todavia no sacarlo) el primero y solicitarle a la memoria su creacion, 

    sem_wait(&sem_long_term);
    log_debug(logger,"HILO CREADOR RETIENE WAIT_LONG_TERM");
    t_pcb * new_process = get_with_mutex(new,&mutex_new,0);// IMPORTANTE, get no lo saca, lo deja en la lista, solo devuelve un puntero al elemento TODAVIA en la lista
        if(create_process_in_memory(new_process)){ //si pude crearlo, entonces ahora si lo quito de new, creo su tid 0 y lo agrego a ready, agrego pcb a lista de pcbs (lista global de pcbs procesos creados)
            log_debug(logger,"Exito al crear el proceso %d en memoria",new_process->pid);
            remove_element_con_mutex(new,new_process,&mutex_new);
            t_tcb* main_thread_tcb = create_tcb(new_process,new_process->main_thread_path,new_process->main_thread_priority);//EN ESTA INVOCACION con pasarle solo el pcb alcanzaría, pasa que a veces el segundo y tercer parametro no son los mismo del pcb //quedan asociados porque tcbs conoce el pid de su "padre"
            change_pcb_state(new_process,EXEC);
            add_tcb_to_state(main_thread_tcb,READY);
            sem_post(&sem_long_term);
        }else{//si NO pude crearlo, NO lo quito (de forma que siga primero porque cuando se libere un proceso, sea este el que mas prioridad tenga) y debo
            log_debug(logger,"ERROR al crear el proceso %d en memoria, dejo de crear hasta que finalice alguno",new_process->pid);
            no_memory_for_new_process=true;// poner el flag no_memory_for_new_process en true
            sem_post(&sem_long_term);//signal binario para creacion de procesos
            sem_wait(&sem_retry_process_creation);// poner un semaforo "sem_retry_process_creation" en WAIT  // OJOOOOOOOOOOOOOOO------------------------>entonces cada vez que se finalice un proceso-> checkeo si no_memory_for_new_process= true, si ese fuera el caso, ponerlo en false y desbloquear el semaforo "carlitos" para volver a intentar crear el proceso
            sem_post(&sem_counter_new);        //una vez desbloqueado el sem_retry_process_creation, hay que aumentar el contador de la cola new porque se supone que no salio ningun pcb de la cola new, simplemente se intento crear un proceso pero no se pudo
        }              
    }
}