
#include <stdio.h>
#include <pthread.h>

#ifndef SUSE_H_
#define SUSE_H_

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
}t_semaforo;

typedef struct {
  int id;
  t_estado estado;
  t_semaforo* semaforos[];
  int tiempoDeEjecucion;
  int tiempoDeEspera;
  int tiempoDeCpu;
}t_hilo;

typedef struct {
  t_list* listaDeHilos ;
  t_queue* colaDeReady;
  t_hilo* enEjecucion;
}t_programa;

#endif /* SUSE_H_ */
