#ifndef FILESYSTEM_CONFIG_H_
#define FILESYSTEM_CONFIG_H_

#include "globals.h"

t_filesystem_config *create_filesystem_config(char *path_a_config);
void destroy_filesystem_config(t_filesystem_config *filesystem_config);

#endif