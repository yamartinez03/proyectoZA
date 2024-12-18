#include "utils.h"

t_config *config;

int close_program(t_log *logger)
{
	log_info(logger, "Program close");
	exit(0);
}

int get_minimun(int first, int second)
{
	if (first < second)
		return first;
	return second;
}

void * pop_con_mutex(t_list* lista, pthread_mutex_t* mutex){
	pthread_mutex_lock(mutex);
	void* elemento = list_remove(lista, 0); // obtiene el PRIMER ELEMENTO de esta cola
	pthread_mutex_unlock(mutex);
	return elemento;
}

void push_con_mutex(t_list* lista, void * elemento ,pthread_mutex_t* mutex){
  	pthread_mutex_lock(mutex);
    list_add(lista, elemento);
	pthread_mutex_unlock(mutex);
    return;
}

void * remove_con_mutex(t_list* lista, pthread_mutex_t* mutex, int posicion){
	pthread_mutex_lock(mutex);
	void* elemento = list_remove(lista, posicion); 
	pthread_mutex_unlock(mutex);
    return elemento;
}

void remove_element_con_mutex(t_list* lista, void *element ,pthread_mutex_t* mutex){
	pthread_mutex_lock(mutex);
	list_remove_element(lista,element);
	pthread_mutex_unlock(mutex);
	return;
}

void * get_with_mutex(t_list* list,pthread_mutex_t * mutex, int index){
	pthread_mutex_lock(mutex);
	void* element = list_get(list,index);
	pthread_mutex_unlock(mutex);
    return element;
}

void write_bool_with_mutex(bool * a_bool, pthread_mutex_t * mutex, bool new_bool_value){
	pthread_mutex_lock(mutex);
	*a_bool=new_bool_value;
	pthread_mutex_unlock(mutex);
};

bool read_bool_with_mutex(bool* a_bool,pthread_mutex_t * mutex){
	pthread_mutex_lock(mutex);
	bool placeholder= *a_bool;
	pthread_mutex_unlock(mutex);
	return placeholder;
};

int get_int_from_list(t_list * list, int index){
    return *((int*)list_get(list,index));
}

uint32_t get_uint32_t_from_list(t_list * list, int index){
    return *((uint32_t*)list_get(list,index));
}

char * get_string_from_list(t_list * list, int index){
    return ((char*)list_get(list,index));
}

int get_atoi_from_list(t_list* list,int index){
	return atoi(get_string_from_list(list,index));
}

uint32_t get_uint32_t_atoi_from_list(t_list* list,int index){
	return ((uint32_t) get_atoi_from_list(list,index));
}

int * get_pointer_to_duplicated_int_from_list(t_list* list ,int index){//devuelve puntero al heap
	int * new_pointer = malloc(sizeof(int));
	*new_pointer = get_int_from_list(list,index);
	return new_pointer;
}

uint32_t * get_pointer_to_duplicated_uint32_t_from_list(t_list* list ,int index){//devuelve puntero al heap
	uint32_t * new_pointer = malloc(sizeof(uint32_t));
	*new_pointer = get_uint32_t_from_list(list,index);
	return new_pointer;
}

char * duplicated_string_from_list(t_list* list ,int index){//devuelve puntero al heap
	char * original = get_string_from_list(list,index);
	char * new_pointer = string_duplicate(original);
	return new_pointer;
}
