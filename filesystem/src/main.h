#ifndef FILSYSTEM_MAIN_H_
#define FILESYSTEM_MAIN_H_

#include "globals.h"
#include <utils/hello.h>
#include <../../utils/src/utils/sockets.h> //en teoria el compilador deberia poder trabajar con #include "sockets.h" ya que el makefile deberia encontrarlo en utils/src/utils
#include "handle_connection.h"
#include "config.h"


void inicializar_archivos();

int server_escuchar();
void terminar_programa();

void abrir_bitmap();
void abrir_archivo_bloques();
void setear_bitmap();

#endif