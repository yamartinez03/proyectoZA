#include "process.h"

const char *translate_thread_state(t_state state)
{
    switch(state)
    {
    case NEW:
        return "NEW";
        break;
    case READY:
        return "READY";
        break;
    case EXEC:
        return "EXEC";
        break;
    case EXIT:
        return "EXIT";
        break;
    case BLOCK:
        return "BLOCK";
        break;
    case NULL_STATE:
        return "NULL_STATE";
        break;
    default:
        return NULL;
        break;
    };
}

bool sort_by_priority(void *first_thread, void *second_thread)// los pongo en void porque si no no se pueden invocar, va creo, asi funcionaba con list find
{
    t_tcb * first_tcb = (t_tcb *) first_thread;
    t_tcb * second_tcb = (t_tcb *) second_thread;
    return first_tcb->priority <= second_tcb->priority;
}
/*
 @brief Agrega un elemento a una lista ordenada, manteniendo el
	*        orden definido por el comparador
	* @param data: El elemento a agregar. Este elemento pasará a pertenecer
	*              a la lista, por lo que no debe ser liberado por fuera de ésta.
	* @param comparator: Funcion que compara dos elementos. Debe devolver
	*                    true si el primer parametro debe aparecer antes que el
	*                    segundo en la lista
*/

