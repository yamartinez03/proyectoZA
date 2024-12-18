#ifndef KERNEL_HANDLE_CONNECTION_H_
#define KERNEL_HANDLE_CONNECTION_H_

#include "globals.h" // lo encuentra en este mismo directorio
//#include <stdio.h> //ya en globals, podría sacarse de aqui?
//#include <stdlib.h>//ya en globals, podría sacarse de aqui?
#include <string.h> 
#include <pthread.h>
//#include <commons/log.h>//ya en globals, podría sacarse de aqui?
//#include <semaphore.h>//ya en globals, podría sacarse de aqui?
//#include <pthread.h>//ya en globals, podría sacarse de aqui?

//incluimos las de utils
#include <../../utils/src/utils/sockets.h>// probar si anda con #include "sockets.h", porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/serialization.h>// probar si anda con #include "serialization.h", porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/operation_handler.h>// probar si anda con #include "operation_handler.h", porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/process.h>// probar si anda con #include "process.h", porque el makefile deberia buscarlo en utils/src/utils


#include <../include/long_term.h>
#include <../include/short_term.h>
//#include "kernel.h" //esto es necesario? creo que no
//#include "semaphores.h"
//#include "queues.h"
//#include "planners.h"

void planner_procedure();

void memory_handshake();


#endif