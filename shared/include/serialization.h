#ifndef SHARED_SERIALIZATION_H_
#define SHARED_SERIALIZATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "operation_handler.h"
#include <commons/log.h>

#define DEFAULT_BUFFER_SIZE 64

typedef struct
{
  int size;
  void *stream;
} t_buffer;
typedef struct
{
  t_operation_code codigo_operacion;
  t_buffer *buffer;
} t_paquete;
;

t_paquete *crear_paquete(t_operation_code OPERACION);
void crear_buffer(t_paquete *paquete);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void *serializar_paquete(t_paquete *paquete, int bytes);
void eliminar_paquete(t_paquete *paquete);
int recibir_operacion(int socket_cliente, t_log *unLogger, char *elQueManda);
void *recibir_buffer(int *size, int socket_cliente);
t_list *recibir_paquete(int socket_cliente);
bool send_handshake(int socket, t_operation_code transmitter, t_operation_code receiver); // se envia un codigo que dice quien manda, y se espera que el servidor responda su propio codigo, ej: kernel le envia el codigo "kernel" a cpu, y cpu responde "cpu", y kernel verifica que sea "cpu" la respuesta de este modulo.

#endif