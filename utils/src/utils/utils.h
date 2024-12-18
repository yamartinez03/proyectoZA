#ifndef SHARED_UTILS_H_
#define SHARED_UTILS_H_

#include <commons/log.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>
#include <stdint.h>

extern t_log *logger;
extern t_config *config;

int close_program(t_log *logger);
int get_minimun(int first, int second);

void* pop_con_mutex(t_list* lista, pthread_mutex_t* mutex);
void push_con_mutex(t_list* lista, void * elemento,pthread_mutex_t* mutex);
void * remove_con_mutex(t_list* lista, pthread_mutex_t* mutex, int posicion);
void remove_element_con_mutex(t_list* lista, void *element ,pthread_mutex_t* mutex);
void * get_with_mutex(t_list* list,pthread_mutex_t * mutex, int index);
void write_bool_with_mutex(bool * a_bool, pthread_mutex_t * mutex, bool new_bool_value);
bool read_bool_with_mutex(bool* a_bool,pthread_mutex_t * mutex);
int get_int_from_list(t_list * list, int index);
uint32_t get_uint32_t_from_list(t_list * list, int index);
char * get_string_from_list(t_list * list, int index);
int get_atoi_from_list(t_list*  list,int index);
uint32_t get_uint32_t_atoi_from_list(t_list* list,int index);
int * get_pointer_to_duplicated_int_from_list(t_list* list ,int index);
uint32_t * get_pointer_to_duplicated_uint32_t_from_list(t_list* list ,int index);
char * duplicated_string_from_list(t_list* list ,int index);
#endif