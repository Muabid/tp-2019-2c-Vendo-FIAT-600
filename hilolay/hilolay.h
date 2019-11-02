#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "net.h"
#include "protocol.h"
#include <semaphore.h>


#define ipServidor "127.0.0.1"
#define puertoServidor 20000




#ifndef HILOLAY_H_
#define HILOLAY_H_

typedef enum {
  BLOCKED = false,
  UNBLOCKED = true
}t_estado_bloqueo;

typedef struct {
	int identificador;
	t_estado_bloqueo estado;
	t_list* semaforos;
}t_hilo;

typedef struct {
	char* nombre;
	int valor;
}t_semaforo;




int hilolay_init();
int suse_create();
int suse_schedule_next();
int suse_wait();
int suse_signal();
void suse_join();


#endif /* HILOLAY_H_ */


