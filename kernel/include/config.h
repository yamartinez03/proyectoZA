#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

//#include <stdio.h> //ya en globals, podría sacarse de aqui?
//#include <stdlib.h>  //ya en globals, podría sacarse de aqui?
//#include <commons/string.h> //ya en globals, podría sacarse de aqui?
//#include <commons/config.h> //ya en globals, podría sacarse de aqui?
#include "globals.h" // lo busca en este mismo directorio

t_kernel_config *create_kernel_config(char *path_a_config);

void destroy_kernel_config(t_kernel_config *kernel_config); 

#endif
