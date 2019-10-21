#include "suse.h"


const int MULTIPROGRAMACION = 3;
int programasEnMemoria = 0;
int listen_port = 20000;

t_queue* colaNuevos; //Se inicializa en main
t_list* listaDeProgramas; //Se inicializa en main


//HELP voy a necesitar una cola de espera, y un productor consumidor, como puede haber muchos hilos ejecutando esto
//como hago los semaforos?


void suseCreate(int threadId, t_programa* padreId) {
	t_hilo* nuevo = malloc(sizeof(t_hilo));
	nuevo->id = threadId;
	nuevo->idPadre = padreId;
	nuevo->estado = NEW;
	nuevo->semaforos = NULL;

	queue_push(colaNuevos, nuevo);
}

void suseScheduleNext(t_programa* programa) {
	if(queue_size(programa->colaDeReady) > 0) {
		t_hilo* hilo = queue_pop(programa->colaDeReady);
		send_message(programa->id, SUSE_SCHEDULE_NEXT, &hilo->id, sizeof(int));
	}
	else
		send_message(programa->id, ERROR_MESSAGE, NULL, 0);
}

void cargarHilosAReady() {
	while(programasEnMemoria < MULTIPROGRAMACION && queue_size(colaNuevos) != 0) {
		t_hilo* hilo = queue_pop(colaNuevos);
		queue_push(hilo->idPadre->colaDeReady, hilo);
	}
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

	while((bufferLoco = recv_message(socket))->head < 7) { // HAY CODIGOS HASTA 5, por eso menor a 6.HAY QUE AGREGAR UNA COLA DE ESPERA
		printf("Se recibió un mensaje\n");
		int threadId = *(int*)bufferLoco->content;
		t_header header = bufferLoco->head;
		size_t tamanio = bufferLoco->size;

		switch(bufferLoco->head) {
			case SUSE_CREATE:
				suseCreate(threadId, programa);
				printf("Se ejecutó SUSE_CREATE\n");
				break;

			case SUSE_SCHEDULE_NEXT:
				cargarHilosAReady();
				suseScheduleNext(programa);
				printf("Se ejecutó SUSE_SCHEDULE_NEXT\n");

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


			case TEST:
				printf("El header es: %i --- El contenido es: %i --- Su tamaño es: %zu\n", header, threadId, tamanio);
				break;

			default:
				printf("La instruccion no es correcta\n");
				break;
		}

	}

	printf("Se ha producido un problema de conexión y el hilo programa se dejará de planificar: %i.\n", bufferLoco->head);

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
			//pthread_join(threadId, NULL);
		}


	}
	if(socketDelCliente < 0) {
		perror("Falló al aceptar conexión");
	}

	close(servidor);
}
