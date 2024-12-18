#ifndef KERNEL_PLANNERS_H_
#define KERNEL_PLANNERS_H_

//#include <pthread.h> //ya en globals, podría sacarse de aqui?
//#include <commons/log.h>//ya en globals, podría sacarse de aqui?

#include "globals.h"
//#include "kernel.h" //esto es necesario? creo que no
#include <commons/collections/queue.h>
//#include "semaphore.h" //ya en globals, podría sacarse de aqui?
#include <../../utils/src/utils/operation_handler.h>// probar si anda con #include "operation_handler.h", porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>


void init_mutex();//QUEDA ACA

void init_lists(); //QUEDA ACA

void init_semaphores(); //QUEDA ACA



#endif