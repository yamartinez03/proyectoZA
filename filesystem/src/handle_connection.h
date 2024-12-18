#ifndef FILESYSTEM_HANDLE_CONNECTION_H_
#define FILESYSTEM_HANDLE_CONNECTION_H_


#include "globals.h"
#include <time.h>

typedef struct{
	int pid;
	int tid;
	int tamanio;
	char* buffer; //Este va a tener todo lo escrito en memoria, no se me ocurre otro nombre ahora. 
}t_datos_archivo;

void process_connection(void *void_socket);

t_datos_archivo* recibir_memory_dump(int socket_cliente);
t_operation_code ejecutar_memory_dump(t_datos_archivo* datos_archivo);

t_operation_code buscar_espacio_bitmap(int bloques_necesarios);
t_list* asignar_bloques_bitmap(int bloques_necesarios, char* nombre_archivo);

char* generar_nombre_metadata(int pid,int tid);

void escribir_archivo_bloques(t_datos_archivo*datos_archivo, t_list* lista_de_bloques_asignados, char* nombre_archivo);
void escribir_parte_del_bloque(int desplazamiento,void* buffer, int tamanio);

void crear_archivo_metadata(char *nombre, int tamanio, int nro_bloque_indice);
void escribir_en_metadata(t_config * metadata, char * path_al_meta,char * key, int valor);

uint32_t* convertir_lista_de_indices_a_buffer_de_indices(t_list* lista);

char* concatenar_a_mount_dir(char * resto);
#endif
