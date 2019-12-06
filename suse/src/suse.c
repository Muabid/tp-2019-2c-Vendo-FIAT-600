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
t_log* logger;
int programasEnMemoria = 0;
int cantidadSemaforos;
t_list* listaSemaforos;
t_list* listaNuevos;
t_list* listaDeBloqueados; //SE VA A USAR PARA ESTADISTICAS??
t_list* listaEnEjecucion;
t_list* listaDeProgramas;

//MUTEX GLOBALES
pthread_mutex_t mutexListaSemaforos;
pthread_mutex_t mutexListaNuevos;
pthread_mutex_t mutexListaDeBloqueados;
pthread_mutex_t mutexListaEnEjecucion;
pthread_mutex_t mutexListaDeProgramas;

pthread_mutex_t modificacionDeHilo;

//DECLARACION DE FUNCIONES
void suseScheduleNext(t_programa* programa);



//AUXILIARES PARA BUSQUEDA
int auxiliarParaId;
int auxiliarParaIdPadre;
char* auxiliarString;
double auxiliarParaBusquedaMenor;
t_list* auxiliarListaParaBusqueda;

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
	cantidadSemaforos = strlen((char*)sem_ids)/sizeof(char*);
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
//FUNCIONES PARA DEBUGGEAR
void funcionLocaParaImprimirId(void* hilo) {
	t_hilo* unHilo = (t_hilo*)hilo;
	printf("%i -", unHilo->id);
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

	printf("Hilos bloqueados: ");
	list_map(listaDeBloqueados, funcionLocaParaImprimirId);
	printf("\n");
	printf("---------\n");
}
//AUXILIARES DE FUNCIONES SUSE

void estimarDuracionHilo(t_hilo* unHilo, double duracion) {
	unHilo->estimadoSJF = duracion * alpha_sjf + (1-alpha_sjf) * (unHilo->estimadoSJF); //ALGORITMO SJF
}
void cargarHilosAReady() {
	while(programasEnMemoria < max_multiprog && list_size(listaNuevos) != 0) {
		t_hilo* hilo = list_remove(listaNuevos, 0);
		hilo->estado = READY;
		list_add(hilo->idPadre->listaDeReady, hilo);
		programasEnMemoria ++;
		printf("Se cargo %i a ready y quedan %i en lista de nuevos y %i programas en memoria\n", hilo->id, list_size(listaNuevos), programasEnMemoria);
		mostrarEstado(hilo->idPadre);
	}
}
t_hilo* bloquearHilo(int threadId, t_programa* padre, t_semaforo* unSemaforo) {
	int contadorDeEncuentrosEnMemoria = 0; //Se usa para saber si el hilo se encontro en más de una lista, algo que no puede suceder
	t_hilo* unHilo;
	auxiliarParaId = threadId;
	auxiliarParaIdPadre = padre->id;
	unHilo = list_find(padre->listaDeHilos, esHiloPorId);
	if(unHilo) {//Comprueba que el hilo esté en el programa
		if(unHilo->estado != BLOCKED) {
			unHilo->estado = BLOCKED;
			if(unSemaforo) {//Esto no ocurre cuando se realiza un suse join ya que no es bloqueado por un semaroo
				queue_push(unSemaforo->colaBloqueo, unHilo);
			}

			if(padre->enEjecucion) {
				if(padre->enEjecucion->id == threadId) {
					contadorDeEncuentrosEnMemoria ++;
					printf("ESTA MIERDA SEGURO ROMPE ACA\n");
					suseScheduleNext(padre);
					programasEnMemoria --;//POLEMICO
				}
			}

			auxiliarParaId = threadId;
			auxiliarParaIdPadre = padre->id;

			if(list_any_satisfy(padre->listaDeReady, esHiloPorId)) {
				contadorDeEncuentrosEnMemoria ++;
				list_remove_by_condition(padre->listaDeReady, esHiloPorId);
			}
			if(list_any_satisfy(listaNuevos, esHiloPorId)) {
				list_remove_by_condition(listaNuevos, esHiloPorId);
			}

			if(contadorDeEncuentrosEnMemoria == 1)
				programasEnMemoria --;



			list_add(listaDeBloqueados, unHilo);
			printf("Se bloqueo al hilo: %i y hay %i programas en memoria\n", unHilo->id, programasEnMemoria);
			mostrarEstado(padre);

			return unHilo;
		}
		else {
			log_error(logger, "El hilo ya se encontraba bloqueado");
			return unHilo;
		}


	}
	else {
		log_error(logger, "No se encontró el hilo a bloquear");
		return NULL;
	}



}
void desbloquearHilo(int threadId, t_programa* padre) {
	auxiliarParaId = threadId;
	auxiliarParaIdPadre = padre->id;
	t_hilo* unHilo = (t_hilo*)list_remove_by_condition(listaDeBloqueados, esHiloPorId);
	if(unHilo)
		list_add(listaNuevos, unHilo);
	else
		log_error(logger, "desbloquearHilo: No se encontró al hilo %i en la lista de bloqueados", threadId);
}

//CREAR Y DESTRUIR UN PROGRAMA
t_programa* crearPrograma(int puerto) {
	static int id = 1;

	t_programa* nuevo = malloc(sizeof(t_programa));
	nuevo->puerto = puerto;
	nuevo->joinCounter = 0;
	nuevo->id = id;
	nuevo->listaDeHilos = list_create();
	nuevo->listaDeReady = list_create();
	nuevo->enEjecucion = NULL;
	id ++;

	return nuevo;
}
void destruirPrograma(t_programa* programa) {
	programasEnMemoria -= list_size(programa->listaDeReady);
	printf("Quedaron %i programas en memoria\n", programasEnMemoria);
	if(programa->enEjecucion != NULL)
		programa->enEjecucion = NULL;
		programasEnMemoria--;


//	auxiliarParaId = programa->id;
//	list_remove_and_destroy_by_condition(listaDeProgramas, esProgramaPorId, free);

	printf("Quedaron %i programas en memoria\n", programasEnMemoria);
	mostrarEstado(programa);
}

//SE EJECUTA AL FINALIZAR SUSE
void sigterm(int sig) {
	log_info(logger, "Se interrumpió la ejecución del proceso");
	log_destroy(logger);
}

//FUNCIONES SUSE
void suseCreate(int threadId, t_programa* padreId) {
	t_hilo* nuevo = malloc(sizeof(t_hilo));
	nuevo->id = threadId;
	nuevo->idPadre = padreId;
	nuevo->joined = false;
	nuevo->estado = NEW;
	nuevo->tiempoInicial = 0;
	nuevo->tiempoEspera = 0;
	nuevo->tiempoCpu = 0;
	nuevo->estimadoSJF = 0;

	list_add(padreId->listaDeHilos, nuevo);
	list_add(listaNuevos, nuevo);
	log_info(logger, "Se creó un nuevo hilo número %i del programa %i", threadId, padreId->id);
}
void suseScheduleNext(t_programa* programa) {
	if(list_size(programa->listaDeReady) > 0) {

		if(programa->enEjecucion) {
			if(programa->enEjecucion->estado != BLOCKED) {
				list_add(programa->listaDeReady, programa->enEjecucion);
				programa->enEjecucion = NULL;
			}
		}

		auxiliarListaParaBusqueda = programa->listaDeReady;
		t_hilo* hilo = list_remove_by_condition(programa->listaDeReady, tieneMenorEstimado);

		if(programa->enEjecucion != NULL) {// Si hay hilo en ejecucion y llega uno nuevo
			programa->enEjecucion->estado = READY;
			list_add(programa->listaDeReady, programa->enEjecucion);
			programa->enEjecucion = NULL;
		}
		if(hilo) {
			hilo->estado = EXEC;
			programa->enEjecucion = hilo;
		}

		send_message(programa->puerto, SUSE_SCHEDULE_NEXT, &programa->enEjecucion->id, sizeof(int));
		printf("Se planifico el hilo: %i\n", hilo->id);
	}
	else if(programa->enEjecucion && programa->enEjecucion->estado != BLOCKED){
		send_message(programa->puerto, SUSE_SCHEDULE_NEXT, &programa->enEjecucion->id, sizeof(int));
	}
	else {
		send_message(programa->puerto, ERROR_MESSAGE, NULL, 0);
		log_error(logger, "Se planifo el hilo: ERROR MESSAGE\n");
	}
}
void suseWait(int threadId, char* nombreSemaforo, t_programa* padre) {

		//Se busca el semaforo
		auxiliarString = malloc(strlen(nombreSemaforo)+1);
		strcpy(auxiliarString, nombreSemaforo);
		t_semaforo* unSemaforo = list_find(listaSemaforos, buscarSemaforoPorNombre);
		free(auxiliarString);

		//Se le resta en uno su valor
		unSemaforo->valor --;


		//Si queda negativo se agrega el thread a la lista de bloqueados
		//Y se le agrega al thread por quien fue bloqueado
		if(unSemaforo->valor < 0) {
			bloquearHilo(threadId, padre, unSemaforo);
		}
}
void suseSignal(int threadId, char* nombreSemaforo, t_programa* padre) {

	t_hilo* unHilo;

	//Se busca el semaforo
	auxiliarString = malloc(strlen(nombreSemaforo)+1);
	strcpy(auxiliarString, nombreSemaforo);
	t_semaforo* unSemaforo = list_find(listaSemaforos, buscarSemaforoPorNombre);
	free(auxiliarString);

	//Se le suma en uno su valor
	unSemaforo->valor ++;

	if(unSemaforo->valor == 0) {
		auxiliarParaId = threadId;
		auxiliarParaIdPadre = padre->id;
		unHilo = list_remove_by_condition(padre->listaDeHilos, esHiloPorId);
		list_add(listaNuevos, unHilo);
		unHilo->estado = NEW;
	}

}
void suseClose(int id, t_programa* programa) {
	auxiliarParaId = id;
	auxiliarParaIdPadre = programa->id;

	//INICIO DESBLOQUEO DE JOIN
	auxiliarParaId = id;
	auxiliarParaIdPadre = programa->id;
	t_hilo* unHilo = (t_hilo*)(list_find(programa->listaDeHilos, esHiloPorId));
	if(unHilo->joined) {
		programa->joinCounter --;
		unHilo->joined = false;
		if(programa->joinCounter == 0)
			desbloquearHilo(0, programa);
	}
	//FIN DESBLOQUEO DE JOIN


	if(programa->enEjecucion->id == id) {
		programa->enEjecucion = NULL;
	}
	else if(list_any_satisfy(programa->listaDeReady, esHiloPorId)) {
		list_remove_by_condition(programa->listaDeReady, esHiloPorId);
	}
	else if(list_any_satisfy(listaNuevos, esHiloPorId)) {
		list_remove_by_condition(listaNuevos, esHiloPorId);
	}
	else {
		log_error(logger, "Suse Close: El hilo a hacerle suseClose no se encuentra en memoria");
	}
	log_info(logger, "Suse Close: Se cerró el hilo %i", id);
	programasEnMemoria --;
}
void suseJoin(int threadId, t_programa* padre) { //TESTEAR
	bloquearHilo(0, padre, NULL);
	printf("Se blqoueo el hilo 0\n");
	auxiliarParaId = threadId;
	auxiliarParaIdPadre = padre->id;
	t_hilo* unHilo = (t_hilo*)(list_find(padre->listaDeHilos, esHiloPorId));
	unHilo->joined = true;
	padre->joinCounter ++;
}

//HANDLER POR HILO
void* handler(void* socketConectado) {
	int socket = *(int*)socketConectado;
	t_programa* programa = crearPrograma(socket);
	list_add(listaDeProgramas, programa);
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
					log_error(logger, "Se recibió en SUSE_WAIT un mensaje que no corresponde a este lugar");
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
	listaDeBloqueados = list_create();
	listaDeProgramas = list_create();

	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(port_server);


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
