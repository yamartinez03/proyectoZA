#ifndef CPU_HANDLE_DISPATCH_CONNECTION_H_
#define CPU_HANDLE_DISPATCH_CONNECTION_H_

#include "globals.h"


void handle_dispatch_connection();

void request_context_to_memory();

void fetch();

char* request_instruction_to_memory();

void decode(char * instruction);

void execute(t_instruction * instruction);

void check_interrupt();

void request_context_update_to_memory();

t_instruction * create_instruction(t_operation_code instruction_code,t_operation_code syscall_code,t_list* parameters_list);

void destroy_instruction(t_instruction * instruction);

bool valid_interruption();

void discard_possible_interruption();

uint32_t * point_to_register(char* register_name);

void set_register(uint32_t * pointer_to_register_to_set, uint32_t * pointer_to_value_to_set);

uint32_t read_register(uint32_t * pointer_to_register_to_read);

uint32_t MMU(uint32_t logic_adress);

void execute_syscall(t_instruction * instruction);

t_operation_code instruction_to_instruction_code(char * parameter);

t_operation_code instruction_to_syscall_code(char * parameter);

void identify_thread(int thread_pid,int thread_tid,int time_in_cpu);

uint32_t * get_register_pointer_from_list(t_list* list,int index);
#endif
