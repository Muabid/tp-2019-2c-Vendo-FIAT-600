#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <net.h>

#ifndef SUSE_H_
#define SUSE_H_


//FUNCIONES DE NET.H
	int init_server(int port){
	int  socket, val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr =INADDR_ANY;
	servaddr.sin_port = htons(port);

	socket = create_socket();
	if (socket < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket,(struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
		return EXIT_FAILURE;
	}

	if (listen(socket, MAX_CLIENTS)< 0) {
		return EXIT_FAILURE;
	}

	return socket;

}

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
}t_semaforo;

typedef struct {
  int id;
  t_estado estado;
  int tiempoDeEjecucion;
  int tiempoDeEspera;
  int tiempoDeCpu;
  t_semaforo* semaforos[];
}t_hilo;

typedef struct {
  t_list* listaDeHilos ;
  t_queue* colaDeReady;
  t_hilo* enEjecucion;
}t_programa;

int iniciarServidor(int puerto) {
  int socket = init_server(puerto);
  if(socket == EXIT_FAILURE) {
    //LOG
  }
  else {
    //lOG
  }

  return socket;
}

#endif /* SUSE_H_ */
