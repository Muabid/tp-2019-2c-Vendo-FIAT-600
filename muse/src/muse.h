#ifndef MUSE_H_
#define MUSE_H_

void* posicionInicialMemoria;
void* posicionInicialSwap;
int const PUERTO;
int const TAMANIO_MEMORIA;
int const TAMANIO_PAGINA;
int const TAMANIO_SWAP;
int const CANT_PAGINAS_MEMORIA;
int const CANT_PAGINAS_SWAP;
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

#endif /* MUSE_H_ */
