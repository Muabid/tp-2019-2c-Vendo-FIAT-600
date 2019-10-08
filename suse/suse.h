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
typedef struct t_programa t_programa;
typedef struct t_hilo t_hilo;

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

struct t_hilo {
  int id;
  t_programa* idPadre;
  t_estado estado;
  //ESTO LO VOY A HACER EN HILOLAY
  //int tiempoDeEjecucion;
  //int tiempoDeEspera;
  //int tiempoDeCpu;
  t_list* semaforos;
}__attribute__((packed));

struct t_programa {
  int id;
  t_list* listaDeHilos ;
  t_queue* colaDeReady;
  t_hilo* enEjecucion;
}__attribute__((packed));



#endif /* SUSE_H_ */
