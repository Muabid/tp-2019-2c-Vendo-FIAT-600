#include "suse.h"

//VARIABLES CONFIGURACION
int listen_port;
int metrics_timer;
int max_multiprog;
char** sem_ids;
char** sem_init_value;
char** sem_max;
double alpha_sjf;

int programasEnMemoria = 0;

t_list* listaNuevos; //Se inicializa en main
t_list* listaDeBloqueados;//Se inicializa en main
t_list* listaEnEjecucion;
t_list* listaExit;
t_log* log;

t_list* listaDeProgramas; //Se inicializa en main TIENE ALGUNA UTILIDAD??


//AUXILIARES PARA BUSQUEDA
int auxiliarParaId;
int auxiliarParaIdPadre;
t_list* auxiliarListaParaBusqueda;
double auxiliarParaBusquedaMenor;


void load_suse_config() {
	t_config* config = config_create("suse.config");
	printf("La cantidad de keys son: %i \n", config_keys_amount(config));
	listen_port = config_get_int_value(config, "LISTEN_PORT");
	metrics_timer = config_get_int_value(config, "METRICS_TIMER");
	max_multiprog = config_get_int_value(config, "MAX_MULTIPROG");

	sem_ids = malloc(sizeof(config_get_array_value(config, "SEM_IDS")));
	sem_ids = config_get_array_value(config, "SEM_IDS");

	sem_init_value = malloc(sizeof(config_get_array_value(config, "SEM_INIT")));
	sem_init_value = config_get_array_value(config, "SEM_INIT");

	sem_max = malloc(sizeof(config_get_array_value(config, "SEM_MAX")));
	sem_max = config_get_array_value(config, "SEM_MAX");

	alpha_sjf = config_get_double_value(config, "ALPHA_SJF");

	printf("%i\n", listen_port);
	printf("%i\n", metrics_timer);
	printf("%i\n", max_multiprog);

	printf("%i\n", *(int*)sem_max[0]); //COMO CARGO LOS ARRAYS Y COMO LOS LEO?
	printf("%i\n", *(int*)sem_max[1]);

	printf("%lf\n", alpha_sjf);

	config_destroy(config);

}



bool esHiloPorId(void* unHilo) {
	t_hilo* casteo = (t_hilo*) unHilo;
	return (casteo->id == auxiliarParaId) && (casteo->idPadre->id == auxiliarParaIdPadre);
}

void suseCreate(int threadId, t_programa* padreId) {
	t_hilo* nuevo = malloc(sizeof(t_hilo));
	nuevo->id = threadId;
	nuevo->idPadre = padreId;
	nuevo->estado = NEW;
	nuevo->tiempoInicial = 0;
	nuevo->tiempoEspera = 0;
	nuevo->tiempoCpu = 0;
	nuevo->estimadoSJF = 0;

	list_add(listaNuevos, nuevo);
	list_add(listaEnEjecucion, nuevo);

}

bool esElMenor(void* hilo) {
	t_hilo* unHilo = (t_hilo*)hilo;
	return auxiliarParaBusquedaMenor <= unHilo->estimadoSJF;
}

bool tieneMenorEstimado(void* hilo) {
	t_hilo* unHilo = (t_hilo*)hilo;
	auxiliarParaBusquedaMenor = unHilo->estimadoSJF;
	return list_all_satisfy(auxiliarListaParaBusqueda, esElMenor);
}

void suseScheduleNext(t_programa* programa) {
	if(list_size(programa->listaDeReady) > 0) {
		auxiliarListaParaBusqueda = programa->listaDeReady;
		t_hilo* hilo = list_remove_by_condition(programa->listaDeReady, tieneMenorEstimado);

		if(programa->enEjecucion != NULL) {
			programa->enEjecucion->estado = READY;
			list_add(programa->listaDeReady, programa->enEjecucion);
			programa->enEjecucion = NULL;
		}

		hilo->estado = EXEC;
		programa->enEjecucion = hilo;

		send_message(programa->id, SUSE_SCHEDULE_NEXT, &hilo->id, sizeof(int));
	}
	else
		send_message(programa->id, ERROR_MESSAGE, NULL, 0);
}

void estimarDuracionHilo(t_hilo* unHilo, double duracion) {
	unHilo->estimadoSJF = duracion * alpha_sjf + (1-alpha_sjf) * (unHilo->estimadoSJF); //ALGORITMO SJF
	printf("Se ejecuto SJF");
}

void cargarHilosAReady() {
	while(programasEnMemoria < max_multiprog && list_size(listaNuevos) != 0) {
		t_hilo* hilo = list_remove(listaNuevos, 0);
		hilo->estado = READY;
		list_add(hilo->idPadre->listaDeReady, hilo);
		programasEnMemoria ++;
	}
}

void suseWait(int threadId, char* semaforo, t_programa* padre) {
}

void suseSignal(int threadId, char* semaforo, t_programa* padre) {

}

void suseClose(int id, t_programa* programa) {
	auxiliarParaId = id;
	auxiliarParaIdPadre = programa->id;
	t_hilo* auxiliar;

	if(list_any_satisfy(listaEnEjecucion, esHiloPorId)) {
		if(programa->enEjecucion->id == id) {
			programa->enEjecucion->estado = EXIT;
			list_add(listaExit, programa->enEjecucion);
			programa->enEjecucion = NULL;
		}
		else if(list_any_satisfy(programa->listaDeReady, esHiloPorId)) {
			auxiliar = list_remove_by_condition(programa->listaDeReady, esHiloPorId);
			auxiliar->estado = EXIT;
			list_add(listaExit, auxiliar);
		}

		else if(list_any_satisfy(listaNuevos, esHiloPorId)) {
			auxiliar = list_remove_by_condition(listaNuevos, esHiloPorId);

		}

		else {
			log_error(log, "El hilo a hacerle suseClose no se encuentra en memoria");

		}

		programasEnMemoria --;
		list_remove_by_condition(listaEnEjecucion, esHiloPorId);


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

	while((bufferLoco = recv_message(socket))->head < 10) { // HAY CODIGOS HASTA 7 + Prueba, por eso menor a 7.HAY QUE AGREGAR UNA COLA DE ESPERA
		printf("Se recibió un mensaje\n");
		int threadId = *(int*)bufferLoco->content;
		char* stringAuxiliar;
		double numeroAux;
		t_header header = bufferLoco->head;
		size_t tamanio = bufferLoco->size;

		switch(bufferLoco->head) {
			case SUSE_CREATE:
				suseCreate(threadId, programa);
				break;

			case SUSE_SCHEDULE_NEXT:
					numeroAux = (double)threadId; //En este caso threadId se comporta como el contenedor de el tiempo real para SJF

					if(programa->enEjecucion != NULL) {
						estimarDuracionHilo(programa->enEjecucion, numeroAux);
					}

					cargarHilosAReady();
					suseScheduleNext(programa);




				break;

			case SUSE_WAIT:
				if((bufferLoco = recv_message(socket))->head == SUSE_CONTENT) {
					stringAuxiliar = malloc(sizeof(bufferLoco->content));
					stringAuxiliar = (char*)bufferLoco->content;
					suseWait(threadId, stringAuxiliar, programa);
					free(stringAuxiliar);
				}
				else {
					log_error(log, "Se recibió en SUSE_SIGNAL un mensaje que no corresponde a este lugar");
					free(stringAuxiliar);
				}
				break;

			case SUSE_SIGNAL:
				if((bufferLoco = recv_message(socket))->head == SUSE_CONTENT) {
					stringAuxiliar = malloc(sizeof(bufferLoco->content));
					stringAuxiliar = (char*)bufferLoco->content;
					suseSignal(threadId, stringAuxiliar, programa);
					free(stringAuxiliar);
				}
				else {
					log_error(log, "Se recibió en SUSE_SIGNAL un mensaje que no corresponde a este lugar");
					free(stringAuxiliar);
				}
				break;

			case SUSE_JOIN:
				//suseJoin();
				break;

			case SUSE_CLOSE:
					suseClose(threadId, programa);
				break;

			case SUSE_CONTENT:
				log_error(log, "SE RECIBIO AL HANDLER UN MENSAJE CON CONTENIDO DESTINADO A OTRO LUGAR");
				break;

			case TEST:
				printf("El header es: %i --- El contenido es: %i --- Su tamaño es: %zu\n", header, threadId, tamanio);
				break;

			default:
				log_error(log, "La instruccion no es correcta\n");
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
	load_suse_config();

	listaDeProgramas = list_create();
	listaDeBloqueados = list_create();
	listaEnEjecucion = list_create();
	listaNuevos = list_create();
	listaExit = list_create();

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
