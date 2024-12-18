#ifndef SHORT_TERM_H_
#define SHORT_TERM_H_

#include "globals.h"
#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/utils.h>
#include "planners.h"

#include "tcb.h"
#include "handle_block.h"
#include "handle_cpu.h"
#include "handle_memory.h"
#include "mutex.h"

void short_term_planner();//ESTE SI O SI ACA

t_tcb * find_finished_tcb_in_ready();

t_tcb * select_thread(); //ESTE SI O SI ACA

t_ready_priority_sublist * get_lowest_priority_queue();

t_ready_priority_sublist * get_priority_list(int priority);

#endif