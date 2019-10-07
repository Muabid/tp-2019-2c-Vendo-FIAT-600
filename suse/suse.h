#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "net.h"
#include "protocol.h"
#include <semaphore.h>
#include <string.h>

#ifndef SUSE_H_
#define SUSE_H_


//FUNCIONES DE NET.H


//FIN FUNCIONES NET.H


typedef enum {
  NEW = 1,
  READY = 2,
  EXEC = 3,
  BLOCKED = 4,
  EXIT = 5
}t_estado;

typedef struct {
  int id;
  int estado;
}__attribute__((packed)) t_semaforo;

typedef struct {
  int id;
  int idPadre;
  t_estado estado;
  //ESTO LO VOY A HACER EN HILOLAY
  //int tiempoDeEjecucion;
  //int tiempoDeEspera;
  //int tiempoDeCpu;
  t_list* semaforos;
}__attribute__((packed)) t_hilo;

typedef struct {
  int id;
  t_list* listaDeHilos ;
  t_queue* colaDeReady;
  t_hilo* enEjecucion;
}__attribute__((packed)) t_programa;



#endif /* SUSE_H_ */
