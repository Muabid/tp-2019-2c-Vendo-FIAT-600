#include "suse.h"


const int MULTIPROGRAMACION = 3;
int programasEnMemoria = 0;
int listen_port = 20000;

t_list* listaNuevos; //Se inicializa en main
t_list* listaDeBloqueados;//Se inicializa en main
//t_list* listaEnEjecucion;
t_log* log;

t_list* listaDeProgramas; //Se inicializa en main TIENE ALGUNA UTILIDAD??


//AUXILIARES PARA BUSQUEDA
int auxiliarParaId = -1;
int auxiliarParaIdPadre;


bool esHiloPorId(void* unHilo) {
	t_hilo* casteo = (t_hilo*) unHilo;
	return (casteo->id == auxiliarParaId) && (casteo->idPadre->id == auxiliarParaIdPadre);
}

void suseCreate(int threadId, t_programa* padreId) {
	t_hilo* nuevo = malloc(sizeof(t_hilo));
	nuevo->id = threadId;
	nuevo->idPadre = padreId;
	nuevo->estado = NEW;
	//nuevo->semaforos = NULL;

	list_add(listaNuevos, nuevo);
}

void suseScheduleNext(t_programa* programa) {
	if(list_size(programa->listaDeReady) > 0) {
		t_hilo* hilo = list_remove(programa->listaDeReady, 0);
		send_message(programa->id, SUSE_SCHEDULE_NEXT, &hilo->id, sizeof(int));
		programasEnMemoria --;
	}
	else
		send_message(programa->id, ERROR_MESSAGE, NULL, 0);
}

void cargarHilosAReady() {
	while(programasEnMemoria < MULTIPROGRAMACION && list_size(listaNuevos) != 0) {
		t_hilo* hilo = list_remove(listaNuevos, 0);
		list_add(hilo->idPadre->listaDeReady, hilo);
		programasEnMemoria ++;
	}
}

void suseWait(int threadId, t_programa* padre) {
	//Si el programa está en ready, al llevarlo a la lista de bloqueados hay que disminuir "programasEnMemoria"
	auxiliarParaId = threadId;
	auxiliarParaIdPadre = padre->id;
	t_hilo* buscado = NULL;
	if((buscado = list_remove_by_condition(listaNuevos, esHiloPorId)) != NULL) {
		list_add(listaDeBloqueados, buscado);
		log_info(log, "Se movió al hilo %i del programa %i de la cola de nuevos a la de bloqueados", threadId, padre->id);

	}
//	else if((buscado = list_remove_by_condition(padre->listaDeReady, esHiloPorId)) != NULL) {
//		list_add(listaDeBloqueados, buscado);
//		log_info(log, "Se movió al hilo %i del programa %i de la cola de ready a la de bloqueados", threadId, padre->id);
//
//	}
//	else if(padre->enEjecucion->id == threadId) {
//		list_add(listaDeBloqueados, padre->enEjecucion);
//		padre->enEjecucion = NULL;
//		log_info(log, "Se movió el hilo %i del programa %i de 'en ejecución' a la lista de bloqueados");
//		programasEnMemoria --;
//	}
}

void suseSignal(int threadId, t_programa* padre) {
	auxiliarParaId = threadId;
	auxiliarParaIdPadre = padre->id;
	t_hilo* buscado = list_remove_by_condition(listaDeBloqueados, esHiloPorId);
	if(buscado == NULL) {
		log_info(log, "No se encontro el hilo %i en la lista de bloqueados", threadId);
	}
	else{
		list_add(listaNuevos, buscado);
		log_info(log, "Se removió el hilo %i de la lista de bloqueados y se movió a la cola de nuevos", threadId);
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
	nuevo->listaDeReady = list_create();
	nuevo->enEjecucion = NULL;

	return nuevo;
}

//void freePrograma(t_programa* programa) {
//	list_destroy_and_destroy_elements(programa->listaDeHilos, freeHilo());
//	queue_destroy_and_destroy_elements(programa->colaDeReady, freeHilo());
//	free(programa->enEjecucion);
//	free(programa);
//}

void sigterm(int sig) {
	log_info(log, "Se interrumpió la ejecución del proceso");
	log_destroy(log);
}

void destruirPrograma(t_programa* programa) {
	list_clean_and_destroy_elements(programa->listaDeHilos, free);
	list_clean_and_destroy_elements(programa->listaDeReady, free);
	free(programa->enEjecucion);
	free(programa);
}

void* handler(void* socketConectado) {
	int socket = *(int*)socketConectado;
	t_programa* programa = crearPrograma(socket);
	list_add(listaDeProgramas, programa);


	t_message* bufferLoco;

	while((bufferLoco = recv_message(socket))->head < 7) { // HAY CODIGOS HASTA 5 + Prueba, por eso menor a 7.HAY QUE AGREGAR UNA COLA DE ESPERA
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
				log_info(log, "Se ejecutó 'cargarHilosAMemoria' y en total hay %i elementos en la lista", list_size(programa->listaDeReady));
				suseScheduleNext(programa);
				printf("Se ejecutó SUSE_SCHEDULE_NEXT\n");

				break;

			case SUSE_WAIT:
				suseWait(threadId, programa);
				break;

			case SUSE_SIGNAL:
				suseSignal(threadId, programa);
				break;

			case SUSE_JOIN:
				//suseJoin();
				break;


			case TEST:
				printf("El header es: %i --- El contenido es: %i --- Su tamaño es: %zu\n", header, threadId, tamanio);
				break;

			default:
				log_info(log, "La instruccion no es correcta\n");
				break;
		}

	}

	log_info(log, "Se ha producido un problema de conexión y el hilo programa se dejará de planificar: %i.\n", bufferLoco->head);
	destruirPrograma(programa);
	free_t_message(bufferLoco);
	//close(servidor);
	return NULL;
}

int main() {
	signal(SIGINT, sigterm);
	log = log_create("log", "log_suse.txt", true, LOG_LEVEL_DEBUG);

	listaDeProgramas = list_create();
	listaDeBloqueados = list_create();
	//listaEnEjecucion = list_create();
	listaNuevos = list_create();

//	log_info(log, "Se crearon exitosamente las estructuras");

	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(listen_port);

	//HAY QUE GUARDAR LOS threadId en alguna lista


	while((socketDelCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion))>=0) {
		pthread_t threadId;
		log_info(log, "Se ha aceptado una conexion: %i\n", socketDelCliente);
		if((pthread_create(&threadId, NULL, handler, (void*) &socketDelCliente)) < 0) {
			log_info(log, "No se pudo crear el hilo");
			return 1;
		} else {
			log_info(log, "Handler asignado\n");
			//pthread_join(threadId, NULL);
		}


	}
	if(socketDelCliente < 0) {
		log_info(log, "Falló al aceptar conexión");
	}

	close(servidor);

	return 0;
}
