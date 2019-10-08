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

typedef struct {
	int identificador;
	void* funcion;
}t_hilo_hilolay;

int _suse_init();
t_hilo_hilolay* _suse_create();
int _suse_schedule_next();
void _suse_wait();
void _suse_signal();
void _suse_join();


#endif /* HILOLAY_H_ */


