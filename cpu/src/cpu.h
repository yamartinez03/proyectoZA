#ifndef CPU_MAIN_H_
#define CPU_MAIN_H_

#include <../../utils/src/utils/utils.h>
#include <../../utils/src/utils/serialization.h>

#include "globals.h"
#include "config.h"
#include "handle_dispatch_connection.h"
#include "handle_interrupt_connection.h"

void init_connections();
void init_semaphores();
void handle_dispatch_connection();

#endif
