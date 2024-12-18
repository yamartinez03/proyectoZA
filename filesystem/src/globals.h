#ifndef FILESYSTEM_GLOBALS_H
#define FILESYSTEM_GLOBALS_H

#include <commons/log.h>
#include <commons/string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <math.h>
#include <sys/mman.h>


#include <../../utils/src/utils/process.h>
#include <../../utils/src/utils/operation_handler.h>
#include <../../utils/src/utils/serialization.h>
#include <../../utils/src/utils/sockets.h>
#include <../../utils/src/utils/utils.h>

typedef struct
{
    t_config *config;
    char *puerto_esucha;
    char *mount_dir;
    int block_size;
    int block_count;
    int retardo_acceso_bloque;
    char *log_level;
    char *path_bitmap;
    char *path_bloques;
}t_filesystem_config;

extern t_log * logger;

extern t_filesystem_config * filesystem_config;

extern int filesystem_listening_socket; //este es el socket servidor mediante el cual escuchamos nuevas conexiones
extern int tamanio_bitmap;
extern int tamanio_filesystem;

extern t_bitarray* bitarray;
extern void* buffer_bitmap;
extern char* buffer_bloques;

extern pthread_mutex_t mutex_archivo_bloques;
extern pthread_mutex_t mutex_bitmap;
extern pthread_mutex_t mutex_filesystem;

void inicializar_semaforos();
#endif