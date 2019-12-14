#ifndef MUSE_H_
#define MUSE_H_
#include <stdio.h>
#include <stdlib.h>
#include "estructuras.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <shared/net.h>
#include <shared/protocol.h>
#include <stdint.h>
#include <sys/socket.h>

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
char* rutaSwapping;
int listener_socket;

void inicializarLogger(char* path);
void cargarConfiguracion();
void inicializarEstructuras(char* rutaSwap);
void inicializarBitmap();
void inicializarMemoriaVirtual(char* rutaSwap);
void inicializarSemaforos();
void* handler_clients(void* socket);
void init_muse_server();

int muse_alloc(char* id, uint32_t tamanio);
int muse_free(char* id, uint32_t dir);
int muse_map(char* id, char* path, uint32_t length, uint32_t flag);
int muse_sync(char* id, uint32_t addr, size_t len);
int muse_unmap(char* id, uint32_t dir);
//void* muse_get(char* id, uint32_t src, size_t n);
int muse_get(char* id, void* dst, uint32_t src, size_t n);
int muse_cpy(char* id, uint32_t dst, void* src, size_t n);
int muse_close(char* id_cliente);

void recursiva(int num);
void recursiva2(int num);
char* pasa_palabra(int cod);

#endif /* MUSE_H_ */
