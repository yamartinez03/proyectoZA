#ifndef MAIN_H_
#define MAIN_H_


#include "globals.h"  //este lo encuentra en este mismo directorio
//#include <stdio.h> //ya en globals, podría sacarse de aqui?
//#include <stdlib.h> //ya en globals, podría sacarse de aqui?
#include <string.h> 
//#include <commons/log.h> //ya en globals, podría sacarse de aqui?
#include <commons/collections/list.h>


//a partir de aca, incluimos todos los modulos en el main
#include <../../utils/src/utils/operation_handler.h>// probar si anda con #include "operation_handler.h", porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/serialization.h> //  probar si anda con #include "serialization.h" porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/utils.h>//  probar si anda con #include "utils.h" porque el makefile deberia buscarlo en utils/src/utils
#include <../../utils/src/utils/process.h>//  probar si anda con #include "process.h" porque el makefile deberia buscarlo en utils/src/utils
#include "handle_connection.h" //este lo encuentra en este mismo directorio
#include "config.h" //este lo encuentra en este mismo directorio
//#include "semaphore.h"
//#include "planners.h"
//#include "sockets.h"
//#include "process.h"
void init_connections();


#endif