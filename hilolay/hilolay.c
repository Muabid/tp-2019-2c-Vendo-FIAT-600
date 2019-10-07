#include "hilolay.h"

int retornarId() {
	static int id = 0;
	id ++;
	return id;
}


int _suse_init(char* ip, int puerto) {
	int conectadoAlServer = connect_to_server(ipServidor, puertoServidor, NULL);
	printf("Se conectó al server\n");
	return conectadoAlServer;
}

t_hilo_hilolay* _suse_create(int conectadoAlServer, void (*funcion)()) {
	t_hilo_hilolay* hiloNuevo = malloc(sizeof(int) + sizeof(void*));
	int nuevoId = retornarId();
	if((send_message(conectadoAlServer, SUSE_CREATE, nuevoId, sizeof(int))) < 0) {
		hiloNuevo->identificador = -1;
		hiloNuevo->funcion = NULL;
		return hiloNuevo;
	}
	else {

		hiloNuevo->identificador = nuevoId;
		hiloNuevo->funcion = *funcion;
		return hiloNuevo;
	}
}

int _suse_schedule_next(int threadId) {
	send_message(threadId, SUSE_SCHEDULE_NEXT, NULL, 0);
	t_message* bufferLoco = recv_message(threadId);
	int numeroDeProceso = *(int*)bufferLoco->content;
	free_t_message(bufferLoco);
	printf("El proceso a ejecutar es el: %i/n", numeroDeProceso);
	return numeroDeProceso;
}
