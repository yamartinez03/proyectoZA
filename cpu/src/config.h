#ifndef CPU_CONFIG_H_
#define CPU_CONFIG_H_

#include "globals.h"

#include <../../utils/src/utils/utils.h>

t_cpu_config *create_cpu_config(char *path_a_config);

void destroy_cpu_config(t_cpu_config *cpu_config);

#endif