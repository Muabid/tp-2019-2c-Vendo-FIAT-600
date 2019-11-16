#include "hilolay.h"

int conectadoAlServer = -1;
t_list* listaDeHilos;
int auxiliarParaBusquedaInt;
char* auxiliarParaBusquedaChar;

time_t start;
time_t end;
bool primeraEjecucion = true;



int hilolay_init(char* ip, int puerto) {
	conectadoAlServer = connect_to_server(ipServidor, puertoServidor, NULL);
	printf("Se conectó al server\n");

	listaDeHilos = list_create();
	start = time(NULL);

	return 0;
	//return conectadoAlServer;
}

int suse_create(int id) {
	if(conectadoAlServer == -1) {
		printf("No se ha iniciado el servidor");
		return -1;
	}
 	if((send_message(conectadoAlServer, SUSE_CREATE, &id, sizeof(int))) < 0) { //VA el&?
		return -1;
	}
	else {
		t_hilo* hiloNuevo = malloc(sizeof(t_hilo));
		hiloNuevo->identificador = id;
		hiloNuevo->semaforos = list_create();
		hiloNuevo->estado = UNBLOCKED;
		list_add(listaDeHilos, hiloNuevo);

		return 0;
	}
}

bool buscarHilo(void* hilo) {
	t_hilo* casteo = malloc(sizeof(t_hilo));
	casteo = (t_hilo*) hilo;
	if((casteo->identificador) == auxiliarParaBusquedaInt) {
		free(casteo);
		return true;
	}
	else {
		free(casteo);
		return false;
	}
}

int suse_schedule_next() {

    start = (int)clock();
    end = (int)clock();

    int tiempo;

    if(primeraEjecucion){
    	primeraEjecucion = false;
    	tiempo = 0;
    }
    else {
    	time(&end);
    	tiempo = (int)(end - start);

    }

	if(conectadoAlServer == -1) {
			printf("No se ha iniciado el servidor");
			return -1;
	}
	send_message(conectadoAlServer, SUSE_SCHEDULE_NEXT, &tiempo, sizeof(int));
	t_message* bufferLoco = recv_message(conectadoAlServer);
	int numeroDeProceso = *(int*)bufferLoco->content;
	free_t_message(bufferLoco);
	printf("El proceso a ejecutar es el: %i\n", numeroDeProceso);
	return numeroDeProceso;
}

int suse_wait(int tid, char* semaforo) {
	send_message(conectadoAlServer, SUSE_WAIT, &tid, sizeof(int));
	send_message(conectadoAlServer, SUSE_CONTENT, semaforo, strlen(semaforo) + 1);
	return 0;
}

int suse_signal(int tid, char* semaforo) {

	send_message(conectadoAlServer, SUSE_SIGNAL, &tid, sizeof(int));
	send_message(conectadoAlServer, SUSE_CONTENT, semaforo, strlen(semaforo) + 1);

	return 0;

}

int suse_close(int tid) {
	send_message(conectadoAlServer, SUSE_CLOSE, &tid, sizeof(int));

	return 0;
}
