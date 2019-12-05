#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/log.h>
#include <shared/net.h>
#include <shared/protocol.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>

#ifndef SUSE_H_
#define SUSE_H_


//FUNCIONES DE NET.H


//FIN FUNCIONES NET.H
typedef struct t_programa t_programa;
typedef struct t_hilo t_hilo;
typedef struct t_paquete t_paquete;

typedef enum {
  NEW = 1,
  READY = 2,
  EXEC = 3,
  BLOCKED = 4,
  EXIT = 5
}t_estado;

//typedef struct {
//  int id;
//  int estado;
//}__attribute__((packed)) t_semaforo;

struct t_hilo {
  int id;
  bool joined;
  t_programa* idPadre;
  t_estado estado;
  double tiempoInicial;
  double tiempoEspera;
  double tiempoCpu;
  double estimadoSJF;
}__attribute__((packed));

struct t_programa {
  int id;
  int joinCounter;
  t_list* listaDeHilos ;
  t_list* listaDeReady;
  t_hilo* enEjecucion;
}__attribute__((packed));

typedef struct {
	char* nombre;
	int valor;
	int valorMaximo;
	t_queue* colaBloqueo;
}t_semaforo;

#endif /* SUSE_H_ */
