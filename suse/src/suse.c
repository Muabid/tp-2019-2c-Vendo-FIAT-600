#include "suse.h"

//VARIABLES CONFIGURACION
int port_server;
int metrics_timer;
int max_multiprog;
char** sem_ids;
char** sem_init_value;
char** sem_max;
double alpha_sjf;

//GLOBALES
int programasEnMemoria = 0;
t_log* logger;
t_list* listaSemaforos;
t_list* listaNuevos;
t_list* listaEnEjecucion;
t_list* listaDeProgramas;
//t_list* listaDeBloqueados; //SE VA A USAR PARA ESTADISTICAS??

//AUXILIARES PARA BUSQUEDA
int auxiliarParaId;
int auxiliarParaIdPadre;
char* auxiliarString;
double auxiliarParaBusquedaMenor;
t_list* auxiliarListaParaBusqueda;

//SEMAFOROS
pthread_mutex_t mutexBusquedaGlobal= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBusquedaSemaforo= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexEstimacionMenor = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutexListaSemaforos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaNuevos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaDeBloqueados = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaEnEjecucion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaDeProgramas = PTHREAD_MUTEX_INITIALIZER;

//DECLARACION DE FUNCIONES
void suseScheduleNext(t_programa* programa);

//FUNCIONES A EJECUTAR EN EL INICIO
void load_suse_config() {
	t_config* config = config_create("suse.config");
	port_server = config_get_int_value(config, "LISTEN_PORT");
	metrics_timer = config_get_int_value(config, "METRICS_TIMER");
	max_multiprog = config_get_int_value(config, "MAX_MULTIPROG");
	sem_ids = config_get_array_value(config, "SEM_IDS");
	sem_init_value = config_get_array_value(config, "SEM_INIT");
	sem_max = config_get_array_value(config, "SEM_MAX");
	alpha_sjf = config_get_double_value(config, "ALPHA_SJF");

	config_destroy(config);
}
void cargarSemaforos() {
	listaSemaforos = list_create();
	int cantidadSemaforos = strlen((char*)sem_ids)/sizeof(char*);
	t_semaforo* semaforoAuxiliar;

	for(int i=0; i<cantidadSemaforos; i++) {
		semaforoAuxiliar = malloc(sizeof(t_semaforo));
		semaforoAuxiliar->nombre = malloc(strlen(sem_ids[i])+1);
		strcpy(semaforoAuxiliar->nombre, sem_ids[i]);
		semaforoAuxiliar->valor = atoi(sem_init_value[i]);
		semaforoAuxiliar->valorMaximo = atoi(sem_max[i]);
		semaforoAuxiliar->colaBloqueo = queue_create();
		list_add(listaSemaforos, semaforoAuxiliar);
	}
}

//FUNCIONES AUXILIARES PARA T_LIST
bool esHiloPorId(void* unHilo) {
	t_hilo* casteo = (t_hilo*) unHilo;
	return (casteo->id == auxiliarParaId) && (casteo->idPadre->id == auxiliarParaIdPadre);
}
bool esProgramaPorId(void* unPrograma) {
	t_programa* casteo = (t_programa*) unPrograma;
	return (casteo->id == auxiliarParaId);
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
bool buscarSemaforoPorNombre(void* semaforo) {
	t_semaforo* unSemaforo = (t_semaforo*)semaforo;
	if(strcmp(unSemaforo->nombre, auxiliarString) == 0) {
		return true;
	}
	else {
		return false;
	}
}
bool perteneceAPrograma(void* hilo) {
	t_hilo* unHilo = (t_hilo*)hilo;
	return auxiliarParaIdPadre == unHilo->idPadre->id;
}

//FUNCIONES PARA DEBUGGEAR
void funcionLocaParaImprimirId(void* hilo) {
	t_hilo* unHilo = (t_hilo*)hilo;
	printf("%i-%i |", unHilo->idPadre->id, unHilo->id);
}

void funcionLocaParaMostarJoined(void* hilo) {
	int unHilo = *(int*)hilo;
	printf("Joined hilo %i\n", unHilo);
}
void mostrarEstado(t_programa* unPrograma) {
	printf("---------\n");
	if(unPrograma->enEjecucion) {
		printf("El programa tiene en ejecucion al hilo %i\n", unPrograma->enEjecucion->id);
	}
	else {
		printf("El programa no tiene hilos en ejecucion\n");
	}

	printf("Hilos en ready: ");
	list_map(unPrograma->listaDeReady, funcionLocaParaImprimirId);
	printf("\n");

	printf("Hilos en nuevos: ");
	list_map(listaNuevos, funcionLocaParaImprimirId);
	printf("\n");

//	printf("Hilos bloqueados: ");
//	list_map(listaDeBloqueados, funcionLocaParaImprimirId);
//	printf("\n");

	list_map(unPrograma->joinedBy, funcionLocaParaMostarJoined);

	printf("---------\n");


}

//FUNCIONES AUXILIARES SUSE
t_hilo* buscarHilo(int idHilo, t_programa* programa) {
	pthread_mutex_lock(&mutexBusquedaGlobal);
	auxiliarParaId = idHilo;
	auxiliarParaIdPadre = programa->id;
	t_hilo* retorno = list_find(programa->listaDeHilos, esHiloPorId);
	pthread_mutex_unlock(&mutexBusquedaGlobal);
	return retorno;
}
t_hilo* removerHiloDeLista(t_hilo* unHilo, t_list* unaLista) {
	pthread_mutex_lock(&mutexBusquedaGlobal);
	auxiliarParaId = unHilo->id;
	auxiliarParaIdPadre = unHilo->idPadre->id;
	t_hilo* retorno = list_remove_by_condition(unaLista, esHiloPorId);
	pthread_mutex_unlock(&mutexBusquedaGlobal);
	return retorno;
}
void agregarHiloAListaNuevos(t_hilo* unHilo) {
	pthread_mutex_lock(&mutexListaNuevos);
	list_add(listaNuevos, unHilo);
	pthread_mutex_unlock(&mutexListaNuevos);
}
t_semaforo* buscarSemaforo(char* nombreSemaforo) {
	pthread_mutex_lock(&mutexBusquedaSemaforo);
	auxiliarString = malloc(strlen(nombreSemaforo)+1);
	strcpy(auxiliarString, nombreSemaforo);
	t_semaforo* unSemaforo = list_find(listaSemaforos, buscarSemaforoPorNombre);
	pthread_mutex_unlock(&mutexBusquedaSemaforo);
	free(auxiliarString);
	return unSemaforo;
}
void estimarDuracionHilo(t_hilo* unHilo, double duracion) {
	unHilo->estimadoSJF = duracion * alpha_sjf + (1-alpha_sjf) * (unHilo->estimadoSJF); //Algoritmo sjf
}
void cargarHilosAReady(t_programa* unPrograma) {
	while(programasEnMemoria < max_multiprog && list_size(listaNuevos) != 0) {
		pthread_mutex_lock(&mutexListaNuevos);
		t_hilo* hilo = list_remove(listaNuevos, 0);
		pthread_mutex_unlock(&mutexListaNuevos);
		hilo->estado = READY;
		list_add(hilo->idPadre->listaDeReady, hilo);
		programasEnMemoria ++;
	}
}
void bloquearHilo(t_hilo* unHilo) {
	switch(unHilo->estado) {
		case EXEC:
			unHilo->idPadre->enEjecucion = NULL;
			break;

		case READY:
			removerHiloDeLista(unHilo, unHilo->idPadre->listaDeReady);

			break;

		case NEW://FALTA MUTEX LISTA NUEVOS?
			removerHiloDeLista(unHilo, listaNuevos);
			break;

		default:
			log_error(logger, "bloquearHilo: (NDPN) no se encuentra el hilo en el estado requerido");

	}
	pthread_mutex_lock(&mutexListaDeBloqueados);
	unHilo->estado = BLOCKED;
//	list_add(listaDeBloqueados, unHilo);
	pthread_mutex_unlock(&mutexListaDeBloqueados);
}
bool pendienteDeEnvios(t_programa* unPrograma) {
	return unPrograma->enEjecucion == NULL && list_is_empty(unPrograma->listaDeReady);
}
void desbloquearHilo(t_hilo* unHilo) {
	if(pendienteDeEnvios(unHilo->idPadre)) {
		unHilo->idPadre->enEjecucion = unHilo;
		//send_message(unHilo->idPadre->puerto, SUSE_SCHEDULE_NEXT, &unHilo->id, sizeof(int));
		suseScheduleNext(unHilo->idPadre);
	}
	 else
		list_add(unHilo->idPadre->listaDeReady, unHilo);


//		removerHiloDeLista(unHilo, listaDeBloqueados);

}

//CREAR Y DESTRUIR UN PROGRAMA
t_programa* crearPrograma(int puerto) {
	static int id = 1;
	t_programa* nuevo = malloc(sizeof(t_programa));
	nuevo->puerto = puerto;
	nuevo->joinedBy = list_create();
	nuevo->id = id;
	nuevo->listaDeHilos = list_create();
	nuevo->listaDeReady = list_create();
	nuevo->enEjecucion = NULL;
	id ++;

	return nuevo;
}

void destruirPrograma(t_programa* programa) {

}

//SE EJECUTA AL FINALIZAR SUSE
void sigterm(int sig) {
	log_info(logger, "Se interrumpió la ejecución del proceso");
	log_destroy(logger);
}

//FUNCIONES SUSE
void suseCreate(int threadId, t_programa* programa) {
	t_hilo* nuevo = malloc(sizeof(t_hilo));
	nuevo->id = threadId;
	nuevo->idPadre = programa;
	nuevo->joined = false;
	nuevo->estado = NEW;
	nuevo->tiempoInicial = 0;
	nuevo->tiempoEspera = 0;
	nuevo->tiempoCpu = 0;
	nuevo->estimadoSJF = 0;

	list_add(programa->listaDeHilos, nuevo);
	agregarHiloAListaNuevos(nuevo);

	log_info(logger, "Se creó un nuevo hilo número %i del programa %i", threadId, programa->id);
	mostrarEstado(programa);
}
void suseScheduleNext(t_programa* programa) {
	//mostrarEstado(programa);
	if(programa->enEjecucion) {
		programa->enEjecucion->estado = READY;
		list_add(programa->listaDeReady, programa->enEjecucion);
		programa->enEjecucion = NULL;
	}
	if(!list_is_empty(programa->listaDeReady)) {
		pthread_mutex_lock(&mutexEstimacionMenor);
		auxiliarListaParaBusqueda = programa->listaDeReady;
		t_hilo* aEnviar = list_remove_by_condition(programa->listaDeReady, tieneMenorEstimado);
		pthread_mutex_unlock(&mutexEstimacionMenor);
		aEnviar->estado = EXEC;
		programa->enEjecucion = aEnviar;
		send_message(programa->puerto, SUSE_SCHEDULE_NEXT, &aEnviar->id, sizeof(int));
	}
}
void suseWait(int threadId, char* nombreSemaforo, t_programa* programa) {
	pthread_mutex_lock(&mutexListaSemaforos);
	t_semaforo* unSemaforo = buscarSemaforo(nombreSemaforo);

	t_hilo* unHilo = buscarHilo(threadId, programa);

	if(unHilo) {
		if(unHilo->estado != BLOCKED) {
			unSemaforo->valor --;
			if(unSemaforo->valor < 0) {
//				pthread_mutex_lock(&mutexListaSemaforos);
				printf("Se bloqueo al hilo %i, por el semaforo %s\n", threadId, nombreSemaforo);
				queue_push(unSemaforo->colaBloqueo, unHilo);
//				pthread_mutex_unlock(&mutexListaSemaforos);
				bloquearHilo(unHilo);
			}
		}
		else
			log_error(logger, "suseWait: el hilo a bloquear ya se encontraba bloqueado");
	}
	else
		log_error(logger, "suseWait: no se encontró el hilo a bloquear");
	pthread_mutex_unlock(&mutexListaSemaforos);

}
void suseSignal(char* nombreSemaforo) {
	pthread_mutex_lock(&mutexListaSemaforos);
	t_semaforo* unSemaforo = buscarSemaforo(nombreSemaforo);

	if(unSemaforo->valor < unSemaforo->valorMaximo)
		unSemaforo->valor ++;

	if(!queue_is_empty(unSemaforo->colaBloqueo)) {
//		pthread_mutex_lock(&mutexListaSemaforos);
		t_hilo* unHilo = queue_pop(unSemaforo->colaBloqueo);
//		pthread_mutex_unlock(&mutexListaSemaforos);
		unHilo->estado = READY;
		desbloquearHilo(unHilo);

	}
	pthread_mutex_unlock(&mutexListaSemaforos);
}


void suseClose(int threadId, t_programa* programa) {
	printf("----------------------SUSE CLOSE ------------------------\n");
	mostrarEstado(programa);

	t_hilo* unHilo = buscarHilo(threadId, programa);
	removerHiloDeLista(unHilo, programa->listaDeHilos);

	//Inicio desbloqueo de join
	if(unHilo->joined) {
		removerHiloDeLista(unHilo, programa->joinedBy);
		unHilo->joined = false;
		if(list_is_empty(programa->joinedBy)) {
			log_info(logger, "------------------------Se desbloquea hilo 0-------------------------");
			t_hilo* hiloCero = buscarHilo(0, programa);
			//desbloquearHilo(hiloCero);
			hiloCero->estado = READY;
			list_add(hiloCero->idPadre->listaDeReady, hiloCero);

		}
	}
	//Fin desbloque de join
	log_info(logger, "El estado es %i", unHilo->estado);
	switch(unHilo->estado) {
		case EXEC:
			programa->enEjecucion = NULL;
			programasEnMemoria --;
			break;
		case READY:
			removerHiloDeLista(unHilo, programa->listaDeReady);
			programasEnMemoria --;
			break;
		case NEW:
			removerHiloDeLista(unHilo, listaNuevos);
			break;
		case BLOCKED:
//			removerHiloDeLista(unHilo, listaDeBloqueados);
			programasEnMemoria --;
			break;
		default:
			log_error(logger, "suseClose: No se encuentra hilo a cerrar");
	}
	log_info(logger, " Se cerro el hilo: %i-%i", programa->id, threadId);
	mostrarEstado(programa);
}


void suseJoin(int threadId, t_programa* programa) {
	t_hilo* unHilo = buscarHilo(threadId, programa);
	if(unHilo) {
		unHilo->joined = true;
		//if(list_is_empty(programa->joinedBy)) {
			t_hilo* hiloCero = buscarHilo(0, programa);
			bloquearHilo(hiloCero);
		//}
		list_add(programa->joinedBy, unHilo);
//		list_add(listaDeBloqueados, unHilo);
	}
}

//HANDLER POR HILO
void* handler(void* socketConectado) {
	int socket = *(int*)socketConectado;
	t_programa* programa = crearPrograma(socket);

	pthread_mutex_lock(&mutexListaDeProgramas);
	list_add(listaDeProgramas, programa);
	pthread_mutex_unlock(&mutexListaDeProgramas);


	t_message* bufferLoco;

	while((bufferLoco = recv_message(socket))->head < 10) { // HAY CODIGOS HASTA 7 + Prueba, por eso menor a 7.HAY QUE AGREGAR UNA COLA DE ESPERA
		int threadId = *(int*)bufferLoco->content;
		char* stringAuxiliar;
		double numeroAux;

		switch(bufferLoco->head) {
			case SUSE_CREATE:
				suseCreate(threadId, programa);
				break;

			case SUSE_SCHEDULE_NEXT:
					numeroAux = (double)threadId; //En este caso threadId se comporta como el contenedor de el tiempo real para SJF

					if(programa->enEjecucion != NULL) {
						estimarDuracionHilo(programa->enEjecucion, numeroAux);
					}
					cargarHilosAReady(programa);
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
					log_error(logger, "Se recibió en SUSE_WAIT un mensaje que no corresponde a este lugar");
					free(stringAuxiliar);
				}
				break;

			case SUSE_SIGNAL:
				if((bufferLoco = recv_message(socket))->head == SUSE_CONTENT) {
					stringAuxiliar = malloc(sizeof(bufferLoco->content));
					stringAuxiliar = (char*)bufferLoco->content;
					suseSignal(stringAuxiliar);
					free(stringAuxiliar);
				}
				else {
					log_error(logger, "Se recibió en SUSE_SIGNAL un mensaje que no corresponde a este lugar");
					free(stringAuxiliar);
				}
				break;

			case SUSE_JOIN:
				suseJoin(threadId, programa);
				break;

			case SUSE_CLOSE:
				suseClose(threadId, programa);
				break;

			case SUSE_CONTENT:
				log_error(logger, "SE RECIBIO AL HANDLER UN MENSAJE CON CONTENIDO DESTINADO A OTRO LUGAR");
				break;

			default:
				log_error(logger, "La instruccion no es correcta\n");
				break;
		}

	}

	destruirPrograma(programa);
	log_info(logger, "Se ha producido un problema de conexión y el hilo programa se dejará de planificar: %i.\n", bufferLoco->head);
	free_t_message(bufferLoco);
	return NULL;
}

int main() {
	signal(SIGINT, sigterm);
	logger = log_create("logger", "log_suse.txt", true, LOG_LEVEL_DEBUG);

	load_suse_config();
	cargarSemaforos();

	listaNuevos = list_create();
//	listaDeBloqueados = list_create();
	listaDeProgramas = list_create();

	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(port_server);

//	//TEST SIN HILOS
//	socketDelCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion);
//	handler(&socketDelCliente);


	while((socketDelCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion))>=0) {
		pthread_t threadId;
		log_info(logger, "Se ha aceptado una conexion: %i\n", socketDelCliente);
		if((pthread_create(&threadId, NULL, handler, (void*) &socketDelCliente)) < 0) {
			log_info(logger, "No se pudo crear el hilo");
			return 1;
		} else {
			log_info(logger, "Handler asignado\n");
		}


	}
	if(socketDelCliente < 0) {
		log_info(logger, "Falló al aceptar conexión");
	}

	close(servidor);

	return 0;
}
