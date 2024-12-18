#include "globals.h"


t_log * logger = NULL;

t_filesystem_config * filesystem_config = NULL;

int filesystem_listening_socket=0;
int tamanio_bitmap = 0;
int tamanio_filesystem = 0 ;

t_bitarray* bitarray = NULL;
void* buffer_bitmap = NULL;
char* buffer_bloques = NULL;

pthread_mutex_t mutex_archivo_bloques;
pthread_mutex_t mutex_bitmap;
pthread_mutex_t mutex_filesystem;

void inicializar_semaforos(){ 
	pthread_mutex_init(&mutex_archivo_bloques, NULL);
    pthread_mutex_init(&mutex_bitmap, NULL);
    pthread_mutex_init(&mutex_filesystem,NULL);
}
