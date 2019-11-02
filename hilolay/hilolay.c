#include "hilolay.h"

int conectadoAlServer = -1;
t_list* listaDeHilos;
int auxiliarParaBusquedaInt;
char* auxiliarParaBusquedaChar;


int hilolay_init(char* ip, int puerto) {
	conectadoAlServer = connect_to_server(ipServidor, puertoServidor, NULL);
	printf("Se conectó al server\n");

	listaDeHilos = list_create();

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

bool buscarSemaforo(void* semaforo) {
	t_semaforo* casteo = malloc(sizeof(t_semaforo));
	casteo = (t_semaforo*) semaforo;
	if(strcmp(casteo->nombre, auxiliarParaBusquedaChar)) { //DATAZO: strcmp devuleve FALSE si la comparacion es correcta
		free(casteo);
		return false;
	}
	else {
		free(casteo);
		return true;
	}
}

int suse_schedule_next() {
	if(conectadoAlServer == -1) {
			printf("No se ha iniciado el servidor");
			return -1;
	}
	send_message(conectadoAlServer, SUSE_SCHEDULE_NEXT, NULL, 0);
	t_message* bufferLoco = recv_message(conectadoAlServer);
	int numeroDeProceso = *(int*)bufferLoco->content;
	free_t_message(bufferLoco);
	printf("El proceso a ejecutar es el: %i\n", numeroDeProceso);
	return numeroDeProceso;
}


int suse_wait(int tid, char* semaforo) {
	//BUSCAR EL SEMAFORO EN LA LISTA DE SEMAFOROS DEL HILO Y DISMINUIR EN UNO SU VALOR
	//SI ES NEGATIVO SE LE ENVIA A SUSE UN MENSAJE PARA BLOQUEAR

	//Se setean variables globales para la búsuqeda del semáforo
	auxiliarParaBusquedaInt = tid;
	auxiliarParaBusquedaChar = malloc(strlen(semaforo) + 1);
	strcpy(auxiliarParaBusquedaChar, semaforo);

	//Se busca el hilo
	t_hilo* unHilo = list_find(listaDeHilos, buscarHilo);

	//Se busca el semaforo y se le decrementa su valor en 1
	t_semaforo* unSemaforo = list_find(unHilo->semaforos, buscarSemaforo);
	unSemaforo->valor --;

	//Si el hilo no está bloqueado y el valor del semaforo es menor a 0 se le manda una señal a suse para bloquear el hilo
	if((unHilo->estado == UNBLOCKED) && (unSemaforo->valor < 0)) {
		send_message(conectadoAlServer, SUSE_WAIT, tid, sizeof(int));
	}

	return 0;
}

bool semaforoDesbloqueado(void* semaforo) {
	t_semaforo* casteo = (t_semaforo*) semaforo;
	return casteo->valor >= 0;
}

bool todosLosSemaforosDesbloqueados(t_hilo* hilo) {
	return list_all_satisfy(hilo->semaforos, semaforoDesbloqueado);
}

int suse_signal(int tid, char* semaforo) {
	//BUSCAR EL SEMAFORO EN LA LISTA DE SEMAFOROS DEL HILO Y AUMENTAR EN UNO SU VALOR
	//SI ***NINGUN *** SEMARO ES NEGATIVO, SE ENVIA UN MENSAJE PARA DESBLOQUEAR

	//Se setean variables globales para la búsuqeda del semáforo
	auxiliarParaBusquedaInt = tid;
	auxiliarParaBusquedaChar = malloc(strlen(semaforo) + 1);
	strcpy(auxiliarParaBusquedaChar, semaforo);

	//Se busca el hilo
	t_hilo* unHilo = list_find(listaDeHilos, buscarHilo);

	//Se busca el semaforo y se le aumenta su valor en 1
	t_semaforo* unSemaforo = list_find(unHilo->semaforos, buscarSemaforo);
	unSemaforo->valor ++;

	if((unHilo->estado == BLOCKED) && todosLosSemaforosDesbloqueados(unHilo)) {
		send_message(conectadoAlServer, SUSE_SIGNAL, tid, sizeof(int));
	}

	return 0;
}
