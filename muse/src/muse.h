#ifndef MUSE_H_
#define MUSE_H_

#include "estructuras.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

void* posicionInicialMemoria;
void* posicionInicialSwap;
int PUERTO;
int TAMANIO_MEMORIA;
int TAMANIO_PAGINA;
int TAMANIO_SWAP;
int CANT_PAGINAS_MEMORIA;
int CANT_PAGINAS_SWAP;
int punteroClock;
uint32_t espacioDisponible;
Bitmap* bitmap;
t_list* listaProgramas;
t_list* listaMapeos;
int fileDescriptor;
pthread_mutex_t mut_bitmap;
pthread_mutex_t mut_espacioDisponible;
pthread_mutex_t mut_listaProgramas;
pthread_mutex_t mut_mapeos;
t_log* logger;

void inicializarLogger(char* path);
void cargarConfiguracion();
void inicializarEstructuras();
void inicializarBitmap();
void inicializarMemoriaVirtual(char* rutaSwap);
void inicializarSemaforos();

#endif /* MUSE_H_ */
