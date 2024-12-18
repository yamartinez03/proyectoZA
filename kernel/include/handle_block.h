#ifndef SHORT_TERM_H
#define SHORT_TERM_H

#include "globals.h"
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>
#include "pcb.h"
#include "tcb.h"
#include "handle_memory.h"
#include "planners.h"

t_io_block_handler * create_io_block_handler(t_tcb * tcb, int miliseconds); //BLOQUEO

void io_handler(); //BLOQUEO

#endif