#ifndef SHARED_PROCESS_H
#define SHARED_PROCESS_H

#include <stdio.h>
#include <string.h>
#include <commons/collections/list.h>
#include "sockets.h"
#include "serialization.h"
#include "operation_handler.h"
#include <semaphore.h>

typedef enum
{
  NEW,
  READY,
  EXEC,
  BLOCK,
  EXIT,
  NULL_STATE //creado para logica nada más, no tienen nigun proposito
} t_state;//no se por que esta en utils, es un tipo de dato que solo maneja el kernel

typedef struct
{//quito todos los mutex y dejo uno solo no? es mejor opcion
  int pid;
  int main_thread_priority;
  t_list *tid_list;
  t_list *mutex_list;
  t_state state;
  char * main_thread_path;
  int tid_counter;
  uint32_t size;
  t_list * tcb_list;
  pthread_mutex_t pcb_access_mutex; //ahora uso un solo semaforo para que cada hilo modifique el tcb solo UNO a la vez, mas simple que un mutex por cada cola // SOLO INVOCAR CUANDO ACCEDEMOS A ELEMENTOS EL PCB QUE PUEDEN CAMBIARSE en otros hilos
} t_pcb; //no se por que esta en utils, es un tipo de dato que solo maneja el kernel


typedef struct
{
  int tid;
  int pid;
  int priority;
  int times_in_cpu;
  int tid_of_tcb_joined_to;
  t_state state;
  char * path;
  t_operation_code block_type;
  t_list * joined_tcbs;
  t_pcb * pcb;
  sem_t sem_finished_thread;
  pthread_mutex_t tcb_access_mutex;//esto sirve para que no NUNCA haya condiciones de carrera con el manejo de un mutex, es mejor que un mutex por cada lista o variable, SOLO INVOCAR cuando accederemos a datos de un tcb que pueden cambiarse en otros hilos
} t_tcb; //no se por que esta en utils, es un tipo de dato que solo maneja el kernel

typedef struct
{
  t_list *blocked_list;
  t_tcb *tcb;
  char * mutex_name;
} t_mutex; //no se por que esta en utils, es un tipo de dato que solo maneja el kernel


/*
una idea para no tener tantos mutex inexplicables dentro de un tipo de dato, es

no usar listas y mutex de forma separada, SI NO, UNA TAD NUEVA:

typedef struct {
pthread_mutex_t mutex_asociado;
t_list *lista;
}


OJO, HABRIA QUE GENERAR TODAS OPERACIONES NUEVAS PARA ESTA TAD
Y APLICARLAS EN TODOS LOS MODULOS, QUIZAS NO VALE LA PENA TODO EL ESFUERZOS
*/


typedef struct {
	uint32_t PC;
  uint32_t AX;
  uint32_t BX;
  uint32_t CX;
  uint32_t DX;
  uint32_t EX;
  uint32_t FX;
  uint32_t GX;
  uint32_t HX;
  
} t_registros_cpu;


const char *translate_thread_state(t_state state);

bool sort_by_priority(void *first_thread, void *second_thread);

#endif