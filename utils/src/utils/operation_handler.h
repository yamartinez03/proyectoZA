#ifndef SHARED_OPERATION_HANDLER_H_
#define SHARED_OPERATION_HANDLER_H_

#include <stdint.h>
typedef enum
{
  KERNEL,
  CPU,
  FILESYSTEM,
  MEMORY,
  SYSCALL,
  INTERRUPTION,
  QUANTUM,
  OK,
  ERROR,
  PROCESS_CREATE,
  PROCESS_EXIT,
  THREAD_CREATE,
  THREAD_JOIN,
  THREAD_CANCEL,
  THREAD_EXIT,
  MUTEX_CREATE,
  MUTEX_LOCK,
  MUTEX_UNLOCK,
  DUMP_MEMORY,
  IO,
  SEGMENTATION_FAULT,
  INSTRUCTION,
  THREAD,
  SET,
  READ_MEM,
  WRITE_MEM,
  SUM,
  SUB,
  JNZ,
  LOG,
  OBTENER_CONTEXTO,
  ACTUALIZAR_CONTEXTO,
  OBTENER_INSTRUCCION,
  FINISH_PROCESS,
  FINISH_THREAD,
  NOT_IN_MEMORY
} t_operation_code;


const char *translate_header(uint8_t operation_code);

// deprecated
// void send_instruction(int client_socket, t_operation_code *instruction);
// void receive_message(int client_socket, char *message);
// void handler_operation(int client_socket, t_operation_code *operation);
#endif