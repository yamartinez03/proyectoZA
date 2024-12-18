#include "handle_dispatch_connection.h"

void handle_dispatch_connection(){
    t_operation_code operacion;//placeholder
    t_operation_code handshake = CPU;//placeholder

    cliente_dispatch = waiting_client(cpu_dispatch_connection);
    if(cliente_dispatch){
        log_debug(logger, "Se conecto KERNEL por DISPATCH");

        while (cliente_dispatch != -1){ 
            sem_wait(&sem_receive_new_thread);
            if(recv(cliente_dispatch,&operacion,sizeof(operacion),MSG_WAITALL) != sizeof(operacion)){
                log_warning(logger, "ERROR en el envio del COD-OP por parte del CLIENTE"); 
            }
            switch (operacion){
            case KERNEL: //esto es solo el handshake
                log_info(logger,"Se conecto KERNEL por DISPATCH");
                send(cliente_dispatch, &handshake, sizeof(t_operation_code),0);
                break;
            case THREAD: 
                t_list * thread_data_received = recibir_paquete(cliente_dispatch);
                int thread_tid = get_int_from_list(thread_data_received,0);
                int thread_pid = get_int_from_list(thread_data_received,1);
                int time_in_cpu = get_int_from_list(thread_data_received,2);//es responsabilidad del kernel saber cuantas veces mando un thread a memoria, asi que, nosotros le hacemos caso y guardamos el dato que nos mande
                list_destroy_and_destroy_elements(thread_data_received,(void *) free);
                
                thread_in_memory=true; //primero asumimos que esta
                log_debug(logger,"Llega el PID %d TID %d por %d vez en CPU",thread_pid,thread_tid,time_in_cpu);
                identify_thread(thread_pid,thread_tid,time_in_cpu);
       
                log_info(logger,"NEW THREAD RECEIVED: PID %d - TID %d",current_thread.pid,current_thread.tid);
                
                request_context_to_memory();

                log_info(logger,"## TID: %d - Solicito Contexto Ejecución",thread_tid);
                
                pthread_t new_instruction_cycle;
                pthread_create(&new_instruction_cycle,0,(void*)fetch,NULL);//debemos hacerlo paralelamente otherwise el no sale de fetch
                pthread_detach(new_instruction_cycle);


                break;
            default:
                log_debug(logger,"Unknown operation from Dispatch");
                break;
            }
        }
    }

}

void fetch(){
    if(thread_in_memory){
        log_info(logger,"## TID: %d - FETCH - Program Counter: %d",current_thread.tid,cpu_registers.pc);
        char* instruction = request_instruction_to_memory();//instruccion = pedir instruccion a memoria// 
        decode(instruction);
    }else{
        char* instruction = NULL;
        decode(instruction);
    }
    
}

void decode(char * instruction){ //recibimos un DUPLICADO de la instruccion
    //con lo que nos pasa memoria, obtenermos un t_operation_code y una lista de parametros--> tincho nos manda la instruccion completa-> nosotros la separamos en una lista
    
    if(instruction){ // se podria verificar con thread_in_memory tambien
        thread_in_memory=true;
        char** instruction_parts = string_split(instruction," "); //supongo que string_split te devuelve un array en el 
    
        free(instruction);// libero el duplicado
        
        t_operation_code instruction_code = instruction_to_instruction_code(instruction_parts[0]);
        t_operation_code syscall_code = instruction_to_syscall_code(instruction_parts[0]);


        log_debug(logger,"INSTRUCTION CODE:%d",instruction_code);
        log_debug(logger,"SYSCALL CODE:%d",syscall_code);


        t_list* parameters_list = list_create(); //la lista se crea, si la syscall no tiene parametros, quedara vacia
        
        for(int i=1; instruction_parts[i] ;i++ ){ // el ultimo elemento sera NULL (marca el final)
            char * dulpicated_parameter = string_duplicate(instruction_parts[i]); //genero un duplicado de cada parametro
            list_add(parameters_list,dulpicated_parameter); 
            log_debug(logger,"Parametro: %d, es:%s",i,dulpicated_parameter);
        }
        
        t_instruction * decoded_instruction = create_instruction(instruction_code,syscall_code,parameters_list);// despues de execute hay que liberar esta memoria

        string_array_destroy(instruction_parts); //o sea, duplicamos los parametros excepto la instruccion, que la hicimos codigo 

        execute(decoded_instruction);
    }else{
        thread_in_memory=false;
        t_instruction * decoded_instruction = NULL;
        execute(decoded_instruction);
    }
    
}

void execute(t_instruction * instruction){ //pensar en alguna abstraccion de logica para las syscalls
    
    if(instruction){//se podria verificar tambien con thread_in_memory
        log_debug(logger,"Codigo de instruccion: %d y codigo de syscall: %d",instruction->instruction_code, instruction->syscall_code);
        t_list * parameters_list = instruction->parameters_list; //OJO, los elementos de esta lista son todos chars

        switch (instruction->instruction_code) //IMPORTANTE-> No eliminar elementos en las listas de parametros, ya que al liberar la instruccion, hacemos list_Destroy_elements
        {
        case SET:{ //Asigna al registro el valor pasado como parámetro.
            uint32_t * pointer_to_register = get_register_pointer_from_list(parameters_list,0);
            uint32_t value = get_uint32_t_atoi_from_list(parameters_list,1);
            log_info(logger,"## TID: %d - Ejecutando: SET %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));
            set_register(pointer_to_register,&value);
            kernel_replanifica=false;
            break;
        }case READ_MEM:{//Lee el valor de memoria correspondiente a la Dirección Lógica que se encuentra en el Registro Dirección y lo almacena en el Registro Datos.
            uint32_t * pointer_to_data_register = get_register_pointer_from_list(parameters_list,0);
            uint32_t logical_adress=read_register(get_register_pointer_from_list(parameters_list,1));
            uint32_t physicall_adress=MMU(logical_adress);

            log_info(logger,"## TID: %d - Ejecutando: READ_MEM %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));

            if(physicall_adress != (-1)){
                log_info(logger,"## TID: %d - Acción: LEER Dirección Física: %u",current_thread.tid,physicall_adress);
                t_operation_code aux;
                t_paquete * paquete = crear_paquete(READ_MEM);
                agregar_a_paquete(paquete,&physicall_adress,sizeof(uint32_t));
                agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
                agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
                enviar_paquete(paquete,memory_connection);

                aux = recibir_operacion(memory_connection);
                aux++; //para que no me rompa los huevos con un warning
                t_list* answer_list = recibir_paquete(memory_connection);
                
                thread_in_memory=true;
                uint32_t value = get_uint32_t_from_list(answer_list,0);

                log_debug(logger,"(NO ES OBLIGATORIO) EL valor que leyo memoria es: %d", value);

                set_register(pointer_to_data_register,&value);
                list_destroy_and_destroy_elements(answer_list,(void*)free);
                eliminar_paquete(paquete);
                
                
                kernel_replanifica=false;
            }else{
                bool buffer;
                t_paquete * paquete = crear_paquete(SEGMENTATION_FAULT);
                agregar_a_paquete(paquete,&(current_thread.pid),sizeof(uint32_t));
                agregar_a_paquete(paquete,&(current_thread.tid),sizeof(uint32_t));
                enviar_paquete(paquete,cliente_dispatch);

                recv(cliente_dispatch,&buffer,sizeof(bool),MSG_WAITALL);

                eliminar_paquete(paquete);
                kernel_replanifica=true;
            }

            
            break;
        }case WRITE_MEM:{//Lee el valor del Registro Datos y lo escribe en la dirección física de memoria obtenida a partir de la Dirección Lógica almacenada en el Registro Dirección.
        
            uint32_t logical_adress=read_register(get_register_pointer_from_list(parameters_list,0));
            uint32_t data_value=read_register(get_register_pointer_from_list(parameters_list,1));
            uint32_t physicall_adress = MMU(logical_adress);

            log_info(logger,"## TID: %d - Ejecutando: WRITE_MEM %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));

            if(physicall_adress != (-1)){
                t_operation_code aux;
                log_info(logger,"## TID: %d - Acción: ESCRIBIR Dirección Física: %u",current_thread.tid,physicall_adress);
                t_paquete * paquete = crear_paquete(WRITE_MEM);
                agregar_a_paquete(paquete,&physicall_adress,sizeof(uint32_t));
                agregar_a_paquete(paquete,&data_value,sizeof(uint32_t));
                agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
                agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));

                log_debug(logger,"(NO ES OBLIGATORIO) EL valor que mande a escribir es: %d", data_value);

                enviar_paquete(paquete,memory_connection);
                recv(memory_connection,&aux,sizeof(t_operation_code),MSG_WAITALL);
                eliminar_paquete(paquete);
                
                thread_in_memory=true;
                
                kernel_replanifica=false;
            }else{

                bool buffer;
                t_paquete * paquete = crear_paquete(SEGMENTATION_FAULT);
                agregar_a_paquete(paquete,&(current_thread.pid),sizeof(uint32_t));
                agregar_a_paquete(paquete,&(current_thread.tid),sizeof(uint32_t));
                enviar_paquete(paquete,cliente_dispatch);

                recv(cliente_dispatch,&buffer,sizeof(bool),MSG_WAITALL);

                eliminar_paquete(paquete);
                kernel_replanifica=true;
            };

            break;
        }case SUM:{//Suma al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.

            log_info(logger,"## TID: %d - Ejecutando: SUM %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));
            
            uint32_t * pointer_to_destination_register = get_register_pointer_from_list(parameters_list,0);
            uint32_t current_destination_value=read_register(pointer_to_destination_register);
            uint32_t origin_value=read_register(get_register_pointer_from_list(parameters_list,1));
            uint32_t new_destination_value = current_destination_value + origin_value;

            set_register(pointer_to_destination_register,&new_destination_value);

            kernel_replanifica=false;

            break;
        }case SUB:{// Resta al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.

            log_info(logger,"## TID: %d - Ejecutando: SUB %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));

            uint32_t * pointer_to_destination_register = get_register_pointer_from_list(parameters_list,0);
            uint32_t current_destination_value=read_register(pointer_to_destination_register);
            uint32_t origin_value=read_register(get_register_pointer_from_list(parameters_list,1));
            uint32_t new_destination_value = current_destination_value - origin_value;
            set_register(pointer_to_destination_register,&new_destination_value);
            kernel_replanifica=false;

            break;
        }case JNZ:{

            log_info(logger,"## TID: %d - Ejecutando: JNZ %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));
            
            uint32_t * pointer_to_register_x = get_register_pointer_from_list(parameters_list,0);
            int new_pc = get_atoi_from_list(parameters_list,1);
            
            uint32_t register_x_value=read_register(pointer_to_register_x);
            
            if(register_x_value){
                cpu_registers.pc = new_pc;
            }

            kernel_replanifica=false;

            break;
        }case LOG:{
            char * register_x = get_string_from_list(parameters_list,0);

            log_info(logger,"## TID: %d - Ejecutando: LOG %s",current_thread.tid,register_x);

            log_info(logger,"[LOG]: %u",read_register(point_to_register(register_x)));

            kernel_replanifica=false;
        
            break;
        }case SYSCALL:{
            execute_syscall(instruction);
            break;
        }default:
            log_debug(logger,"Invalid instruction code");
            abort();
            break;
        }

        destroy_instruction(instruction);
    }else{
        thread_in_memory = false; //x las dudas
    }
    
    if(!thread_in_memory){
        //si fallo antes de execute-> entra
            //si no ->
                //si es syscall no hace mas operaciones a memoria -> no entra
                //si es instruccion
                    //si falla en read mem o write mem entra -> en realidad write y read no pueden fallar porque no entran a memoria proceso->no entra
                    // para cualquier otra no entra
        kernel_replanifica=true;
        bool trash_value;
        t_paquete * paquete = crear_paquete(NOT_IN_MEMORY);
        agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
        agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
        enviar_paquete(paquete,cliente_dispatch);
        recv(cliente_dispatch,&trash_value,sizeof(bool), MSG_WAITALL);//aca recibimos si habra desalojo
        eliminar_paquete(paquete);
    }

    check_interrupt();

}

void check_interrupt(){
    if (kernel_replanifica){
        bool buffer=true;
        log_info(logger,"(%d,%d) - Actualizo Contexto Ejecución",current_thread.pid, current_thread.tid);
        request_context_update_to_memory();
        discard_possible_interruption();
        send(cliente_dispatch,&buffer,sizeof(bool),0);//para desbloquear kernel
        sem_post(&sem_receive_new_thread);
    }else{
        if(valid_interruption()){ //if en todo este tiempo recibi una interrupcion de quantum
            log_debug(logger,"Hay interrupcion valida-> PID: %d TID: %d",current_thread.pid,current_thread.tid);
            request_context_update_to_memory();
            t_paquete * paquete = crear_paquete(INTERRUPTION);
            agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
            agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
            enviar_paquete(paquete,cliente_dispatch);
            sem_post(&sem_receive_new_thread);
            eliminar_paquete(paquete);
        }else{
            fetch();
        }
    }
}

void request_context_to_memory(){
    t_paquete* paquete = crear_paquete(OBTENER_CONTEXTO);
    agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
    agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
    log_debug(logger,"Pido el contexto para el (%d;%d)",current_thread.pid,current_thread.tid);
    t_operation_code aux;

    enviar_paquete(paquete,memory_connection);

    aux=recibir_operacion(memory_connection);
    t_list * answer_list = recibir_paquete(memory_connection);

    if(aux==NOT_IN_MEMORY){
        thread_in_memory=false;
    }else{
        memcpy(&(cpu_registers.pc),list_get(answer_list,0),sizeof(uint32_t));
        memcpy(&(cpu_registers.ax),list_get(answer_list,1),sizeof(uint32_t));
        memcpy(&(cpu_registers.bx),list_get(answer_list,2),sizeof(uint32_t));
        memcpy(&(cpu_registers.cx),list_get(answer_list,3),sizeof(uint32_t));
        memcpy(&(cpu_registers.dx),list_get(answer_list,4),sizeof(uint32_t));
        memcpy(&(cpu_registers.ex),list_get(answer_list,5),sizeof(uint32_t));
        memcpy(&(cpu_registers.fx),list_get(answer_list,6),sizeof(uint32_t));
        memcpy(&(cpu_registers.gx),list_get(answer_list,7),sizeof(uint32_t));
        memcpy(&(cpu_registers.hx),list_get(answer_list,8),sizeof(uint32_t));
        memcpy(&(cpu_registers.base),list_get(answer_list,9),sizeof(uint32_t));
        memcpy(&(cpu_registers.limite),list_get(answer_list,10),sizeof(uint32_t));
        log_debug(logger,"Llego el contexto de ejecucion, el PC:%d",cpu_registers.pc);
        thread_in_memory=true;
    }
    //guardados los valores, libero la informacion de lo que me paso la memoria
    eliminar_paquete(paquete);
    list_destroy_and_destroy_elements(answer_list,(void*)free);
}

char* request_instruction_to_memory(){
    t_paquete * paquete = crear_paquete(OBTENER_INSTRUCCION);
    agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
    agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
    agregar_a_paquete(paquete,&(cpu_registers.pc),sizeof(int));

    t_operation_code aux;
    enviar_paquete(paquete,memory_connection);

    aux=recibir_operacion(memory_connection);
    t_list * answer_list = recibir_paquete(memory_connection);

    if(aux==NOT_IN_MEMORY){
        thread_in_memory=false;
        char *duplicated_instruction=NULL;
        log_debug(logger,"Hilo no mas en memoria");
        eliminar_paquete(paquete);
        list_destroy_and_destroy_elements(answer_list,(void*)free);
        return duplicated_instruction;
    }else{
        thread_in_memory=true;
        char * duplicated_instruction = duplicated_string_from_list(answer_list,0);//devolveremos un DUPLICADO (en heap) de la instruccion completa
        cpu_registers.pc++;//la proxima vez que hagamos fetch, ya aumentamos el numero
        log_debug(logger,"Instruccion recibida: %s", duplicated_instruction);
        eliminar_paquete(paquete);
        list_destroy_and_destroy_elements(answer_list,(void*)free);
        return duplicated_instruction;
    }

}

t_instruction * create_instruction(t_operation_code instruction_code,t_operation_code syscall_code,t_list* parameters_list){ // cambiar el tipo de instrucion code---> coordinar con kernel 
    t_instruction * new_instruction = malloc(sizeof(t_instruction));
    new_instruction->instruction_code = instruction_code; //operaciones
    new_instruction->syscall_code=syscall_code;
    new_instruction->parameters_list= parameters_list;
    return new_instruction;
}

void destroy_instruction(t_instruction * instruction){ //ojo, todos los elementos deben estar en malloc
    list_destroy_and_destroy_elements(instruction->parameters_list,free); // una lista que guarda ints y char* , puede destruirse con el destroyet free? preguntar en foro
    free(instruction);
}

bool valid_interruption(){//si interrupcion llega antes-> la atendemos, si llega despues-> quedara para otro ciclo
    pthread_mutex_lock(&mutex_current_interruption);
    bool return_value = (current_interruption.tid == current_thread.tid) && (current_interruption.pid == current_thread.pid) && (current_interruption.time_in_cpu == current_thread.time_in_cpu); 
    pthread_mutex_unlock(&mutex_current_interruption);
    return return_value;
}

void discard_possible_interruption(){
    pthread_mutex_lock(&mutex_current_interruption);
    log_info(logger,"## Descarto interrupcion PID: %d TID: %d T.I.C: %d",current_interruption.pid,current_interruption.tid,current_interruption.time_in_cpu);
    current_interruption.tid=(-1);
    current_interruption.pid=(-1);
    current_interruption.time_in_cpu=(-1);
    pthread_mutex_unlock(&mutex_current_interruption);
}

uint32_t * point_to_register(char* register_name){ //como solo usa else if, tratar de invocar lo menos posible
    if(!strcmp(register_name,"AX")){
        return &(cpu_registers.ax);
    }else if(!strcmp(register_name,"BX")){
        return &(cpu_registers.bx);
    }else if(!strcmp(register_name,"CX")){
        return &(cpu_registers.cx);
    }else if(!strcmp(register_name,"DX")){
        return &(cpu_registers.dx);
    }else if(!strcmp(register_name,"EX")){
        return &(cpu_registers.ex);
    }else if(!strcmp(register_name,"FX")){
        return &(cpu_registers.fx);
    }else if(!strcmp(register_name,"GX")){
        return &(cpu_registers.gx);
    }else if(!strcmp(register_name,"HX")){
        return &(cpu_registers.hx);
    }else if(!strcmp(register_name,"BASE")){
        return &(cpu_registers.base);
    }else if(!strcmp(register_name,"LIMITE")){
        return &(cpu_registers.limite);
    }

    return NULL;
    
}

void set_register(uint32_t * pointer_to_register_to_set, uint32_t * pointer_to_value_to_set){//solamente para que sea mas legible
    memcpy(pointer_to_register_to_set,pointer_to_value_to_set,sizeof(uint32_t));
}

uint32_t read_register(uint32_t * pointer_to_register_to_read){//solamente para que sea mas legible
    return *pointer_to_register_to_read;
}

uint32_t MMU(uint32_t logic_adress){
    uint32_t base = read_register(point_to_register("BASE"));
    uint32_t limite = read_register(point_to_register("LIMITE"));
    #define REGISTER_SIZE_IN_BYTES 4 //porque es un uint32_t
    uint32_t direccion_fisica = base + logic_adress;

    if(direccion_fisica < limite && direccion_fisica+REGISTER_SIZE_IN_BYTES<=limite){
        return direccion_fisica;
    }else{
        log_info(logger, "SEGMENTATION FAULT"); //Raro que no sea un log obligatorio
        return (uint32_t)(-1);
    }

}


void execute_syscall(t_instruction * instruction){
    t_list* parameters_list = instruction->parameters_list;
    
    log_debug(logger,"(%d)(%d)(%d)",current_thread.pid,current_thread.tid,instruction->syscall_code);

    t_paquete * paquete = crear_paquete(SYSCALL);
    agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
    agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
    agregar_a_paquete(paquete,&(instruction->syscall_code),sizeof(int));
    
    switch(instruction->syscall_code){ //si todas las funciones envian como parametro toda la lista de parametros-> fijate si la funcion paquete() del tp0 sirve para borrar este while
        case DUMP_MEMORY:{//DONE
            log_info(logger,"## TID: %d - Ejecutando: MEMORY_DUMP",current_thread.tid);
            //no agregamos nada más, porque ni siquiera hay lista de parametros
            break;
        }case IO:{
            int time = get_atoi_from_list(parameters_list,0);
            agregar_a_paquete(paquete,&time,sizeof(int));
            log_info(logger,"## TID: %d - Ejecutando: IO %s ",current_thread.tid,get_string_from_list(parameters_list,0));
            break;
        }case PROCESS_CREATE:{//DONE
            char * instructions_file = get_string_from_list(parameters_list,0);
            uint32_t size = get_uint32_t_atoi_from_list(parameters_list,1);
            int thread_0_priority = get_atoi_from_list(parameters_list,2);

            agregar_a_paquete(paquete,instructions_file,string_length(instructions_file)+1);//chequear con memoria si es +1 o no
            agregar_a_paquete(paquete,&size,sizeof(uint32_t));
            agregar_a_paquete(paquete,&thread_0_priority,sizeof(int));

            log_info(logger,"## TID: %d - Ejecutando: PROCESS_CREATE %s %s %s ",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1),get_string_from_list(parameters_list,2));

            break;
        }case THREAD_CREATE:{//DONE
            char * instructions_file = get_string_from_list(parameters_list,0);
            int thread_priority = get_atoi_from_list(parameters_list,1);

            agregar_a_paquete(paquete,instructions_file,string_length(instructions_file)+1);//chequear con memoria si es +1 o no
            agregar_a_paquete(paquete,&thread_priority,sizeof(int));

            log_info(logger,"## TID: %d - Ejecutando: THREAD_CREATE %s %s",current_thread.tid,get_string_from_list(parameters_list,0),get_string_from_list(parameters_list,1));
            
            break;
        }case THREAD_JOIN:{//DONE
            int tid = get_atoi_from_list(parameters_list,0);

            agregar_a_paquete(paquete,&tid,sizeof(int));

            log_info(logger,"## TID: %d - Ejecutando: THREAD_JOIN %s",current_thread.tid,get_string_from_list(parameters_list,0));

            break;
        }case THREAD_CANCEL:{//DONE
            int tid = get_atoi_from_list(parameters_list,0);

            agregar_a_paquete(paquete,&tid,sizeof(int));

            log_info(logger,"## TID: %d - Ejecutando: THREAD_CANCEL %s",current_thread.tid,get_string_from_list(parameters_list,0));
            
            break;
        }case MUTEX_CREATE:{
            char * mutex_name = get_string_from_list(parameters_list,0);

            agregar_a_paquete(paquete,mutex_name,string_length(mutex_name)+1);//chequear con memoria si es +1 o no

            log_info(logger,"## TID: %d - Ejecutando: MUTEX_CREATE %s",current_thread.tid,mutex_name);
            
            break;
        }case MUTEX_LOCK:{
            char * mutex_name = get_string_from_list(parameters_list,0);

            agregar_a_paquete(paquete,mutex_name,string_length(mutex_name)+1);//chequear con memoria si es +1 o no

            log_info(logger,"## TID: %d - Ejecutando: MUTEX_LOCK %s",current_thread.tid,mutex_name);
            
            break;
        }case MUTEX_UNLOCK:{
            char * mutex_name = get_string_from_list(parameters_list,0);

            agregar_a_paquete(paquete,mutex_name,string_length(mutex_name)+1);//chequear con memoria si es +1 o no

            log_info(logger,"## TID: %d - Ejecutando: MUTEX_UNLOCK %s",current_thread.tid,mutex_name);
            
            break;
        }case THREAD_EXIT:{
            log_info(logger,"## TID: %d - Ejecutando: THREAD_EXIT",current_thread.tid);
            //no hay mas parametros
            break;
        }case PROCESS_EXIT:{
            //no hay mas parametros
            log_info(logger,"## TID: %d - Ejecutando: PROCESS_EXIT",current_thread.tid);
            break;
        }default:{
            log_info(logger,"Syscall code desconocido: %d",instruction->syscall_code);
            abort();
        }
    }

    enviar_paquete(paquete,cliente_dispatch);
    recv(cliente_dispatch,&kernel_replanifica,sizeof(bool), MSG_WAITALL);//aca recibimos si habra desalojo
    log_debug(logger,"Syscall atendida: Kernel replanifica? %d",kernel_replanifica);
    eliminar_paquete(paquete);
    
}

t_operation_code instruction_to_instruction_code(char * parameter){
    if(!strcmp(parameter,"SET")){
        return SET;
    }else if(!strcmp(parameter,"READ_MEM")){
        return READ_MEM;
    }else if(!strcmp(parameter,"WRITE_MEM")){
        return WRITE_MEM;
    }else if(!strcmp(parameter,"SUM")){
        return SUM;
    }else if(!strcmp(parameter,"SUB")){
        return SUB;
    }else if(!strcmp(parameter,"JNZ")){
        return JNZ;
    }else if(!strcmp(parameter,"LOG")){
        return LOG;
    }else{
        return SYSCALL;
    }
}

t_operation_code instruction_to_syscall_code(char * parameter){
    if(!strcmp(parameter,"DUMP_MEMORY")){
        return DUMP_MEMORY;
    }else if(!strcmp(parameter,"IO")){
        return IO;
    }else if(!strcmp(parameter,"PROCESS_CREATE")){
        return PROCESS_CREATE;
    }else if(!strcmp(parameter,"THREAD_CREATE")){
        return THREAD_CREATE;
    }else if(!strcmp(parameter,"THREAD_JOIN")){
        return THREAD_JOIN;
    }else if(!strcmp(parameter,"THREAD_CANCEL")){
        return THREAD_CANCEL;
    }else if(!strcmp(parameter,"MUTEX_CREATE")){
        return MUTEX_CREATE;
    }else if(!strcmp(parameter,"MUTEX_LOCK")){
        return MUTEX_LOCK;
    }else if(!strcmp(parameter,"MUTEX_UNLOCK")){
        return MUTEX_UNLOCK;
    }else if(!strcmp(parameter,"THREAD_EXIT")){
        return THREAD_EXIT;
    }else if(!strcmp(parameter,"PROCESS_EXIT")){
        return PROCESS_EXIT;
    }else{
        return INSTRUCTION;
    }
}

void request_context_update_to_memory(){
    //pensar si deberia esperar a la respuesta de memora o hacerse paralelamente

    t_paquete * paquete = crear_paquete(ACTUALIZAR_CONTEXTO);
    agregar_a_paquete(paquete,&(current_thread.pid),sizeof(int));
    agregar_a_paquete(paquete,&(current_thread.tid),sizeof(int));
    
    agregar_a_paquete(paquete,&(cpu_registers.pc),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.ax),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.bx),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.cx),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.dx),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.ex),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.fx),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.gx),sizeof(uint32_t));
    agregar_a_paquete(paquete,&(cpu_registers.hx),sizeof(uint32_t));
    
    t_operation_code buffer;
    enviar_paquete(paquete,memory_connection);
    buffer =recv(memory_connection,&buffer,sizeof(t_operation_code),MSG_WAITALL); //recibimos la respuesta, que no utilizamos para nada
    //no me interesa si encuentra el hilo
    eliminar_paquete(paquete);
}


void identify_thread(int thread_pid,int thread_tid,int time_in_cpu){

    current_thread.pid = thread_pid;
    current_thread.tid = thread_tid;
    current_thread.time_in_cpu =time_in_cpu; 
            
}

uint32_t * get_register_pointer_from_list(t_list* list,int index){ //devuelve el puntero apuntado directamente desde la lista
    char * register_name = get_string_from_list(list,index);
    log_debug(logger,"Register referenced: %s",register_name);
    uint32_t *register_pointer = point_to_register(register_name);
    log_debug(logger,"Current value: %u",read_register(register_pointer));
    return register_pointer;
}
