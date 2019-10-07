#include "suse.h"

int listen_port = 20000;
t_list* listaDeProgramas; //Se inicializa en main
t_queue* colaNuevos; //Se inicializa en main

//void handler(void* socketConectado) {
//	int socket = *(int*)socketConectado;
//	t_message* bufferLoco = recv_message(socket);
//	printf("El mensaje dice: %s\n", (char*) bufferLoco->content);
//	free_t_message(bufferLoco);
//}


//HELP voy a necesitar una cola de espera, y un productor consumidor, como puede haber muchos hilos ejecutando esto
//como hago los semaforos?


void suseCreate(int threadId, int padreId) {
	t_hilo* nuevo = malloc(sizeof(t_hilo));
	nuevo->id = threadId;
	nuevo->idPadre = padreId;
	nuevo->estado = NEW;
	nuevo->semaforos = NULL;

	queue_push(colaNuevos, nuevo);
}

//void freeHilo(t_hilo* hilo) {
//	free(hilo->semaforos);
//	free(hilo);
//}


t_programa* crearPrograma(int id) {
	t_programa* nuevo = malloc(sizeof(t_programa));
	nuevo->id = id;
	nuevo->listaDeHilos = list_create();
	nuevo->colaDeReady = queue_create();
	nuevo->enEjecucion = NULL;

	return nuevo;
}

//void freePrograma(t_programa* programa) {
//	list_destroy_and_destroy_elements(programa->listaDeHilos, freeHilo());
//	queue_destroy_and_destroy_elements(programa->colaDeReady, freeHilo());
//	free(programa->enEjecucion);
//	free(programa);
//}


void* handler(void* socketConectado) {
	int socket = *(int*)socketConectado;
	t_programa* programa = crearPrograma(socket);
	list_add(listaDeProgramas, programa);


	t_message* bufferLoco;

//	t_header header = bufferLoco->head;
//	int mensaje = *(int*)bufferLoco->content;
//	size_t tamanio = bufferLoco->size;
//
//	printf("El header es: %i\nEl mensaje es: %i\nEl tamaño es: %zu\n", header, mensaje, tamanio);

	while((bufferLoco = recv_message(socket))->head < 6) { // HAY CODIGOS HASTA 5, por eso menor a 6.HAY QUE AGREGAR UNA COLA DE ESPERA
		printf("Se recibió un mensaje");
		int threadId = *(int*)bufferLoco->content;

		switch(bufferLoco->head) {
			case SUSE_CREATE:
				suseCreate(threadId, programa->id);
				break;

			case SUSE_SCHEDULE_NEXT:
				//suseScheduleNext();
				break;

			case SUSE_WAIT:
				//suseWait();
				break;

			case SUSE_SIGNAL:
				//suseSignal();
				break;

			case SUSE_JOIN:
				//suseJoin();
				break;

			default:
				printf("La instruccion no es correcta\n");
				break;
		}

	}

	printf("Se ha producido un problema de conexión y el hilo programa se dejará de planificar.\n");

	free_t_message(bufferLoco);
	return NULL;
}

int main() {

	listaDeProgramas = list_create();
	colaNuevos = queue_create();

	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(listen_port);

	//HAY QUE GUARDAR LOS threadId en alguna lista


	while((socketDelCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion))>=0) {
		pthread_t threadId;
		printf("Se ha aceptado una conexion: %i\n", socketDelCliente);
		if((pthread_create(&threadId, NULL, handler, (void*) &socketDelCliente)) < 0) {
			perror("No se pudo crear el hilo");
			return 1;
		} else {
			printf("Handler asignado\n");
			pthread_join(threadId, NULL);
		}


	}
	if(socketDelCliente < 0) {
		perror("Falló al aceptar conexión");
	}

	close(servidor);
}
