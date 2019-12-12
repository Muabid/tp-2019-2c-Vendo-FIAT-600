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
t_list* listaDeBloqueados; //SE VA A USAR PARA ESTADISTICAS??

//AUXILIARES PARA BUSQUEDA
int auxiliarParaId;
int auxiliarParaIdPadre;
char* auxiliarString;
double auxiliarParaBusquedaMenor;
t_list* auxiliarListaParaBusqueda;

//SEMAFOROS
sem_t bloqueoCarga;

pthread_mutex_t mutexBusquedaGlobal= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBusquedaSemaforo= PTHREAD_MUTEX_INITIALIZER;

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
void inicializarSemaforos() {
	sem_init(&bloqueoCarga, 0, 0);
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
			log_error(logger, "bloquearHilo: (NDPN) se encuentra el hilo en NEW");
//			removerHiloDeLista(unHilo, listaNuevos);
			break;

		default:
			log_error(logger, "bloquearHilo: (NDPN) no se encuentra el hilo en el estado requerido");

	}
	pthread_mutex_lock(&mutexListaDeBloqueados);
	unHilo->estado = BLOCKED;
	list_add(listaDeBloqueados, unHilo);
	pthread_mutex_unlock(&mutexListaDeBloqueados);
}
void desbloquearHilo(t_hilo* unHilo) {
	list_add(unHilo->idPadre->listaDeReady, unHilo);
	if(unHilo->estado == BLOCKED)
		removerHiloDeLista(unHilo, listaDeBloqueados);
	else
		log_error(logger, "desbloquearHilo: (NDPN)el hilo a desbloquear no estaba bloqueado");

	unHilo->estado = READY;
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
}
void suseScheduleNext(t_programa* programa) {
	if(programa->enEjecucion) {
		programa->enEjecucion->estado = READY;
		list_add(programa->listaDeReady, programa->enEjecucion);
		programa->enEjecucion = NULL;
	}
	//CONTINUAR
}

void suseWait(int threadId, char* nombreSemaforo, t_programa* programa) {
	t_semaforo* unSemaforo = buscarSemaforo(nombreSemaforo);

	t_hilo* unHilo = buscarHilo(threadId, programa);

	if(unHilo) {
		if(unHilo->estado != BLOCKED) {
			unSemaforo->valor --;
			if(unSemaforo->valor < 0) {
				pthread_mutex_lock(&mutexListaSemaforos);
				queue_push(unSemaforo->colaBloqueo);
				pthread_mutex_unlock(&mutexListaSemaforos);
				bloquearHilo(unHilo);
			}
		}
		else
			log_error(logger, "suseWait: el hilo a bloquear ya se encontraba bloqueado");
	}
	else
		log_error(logger, "suseWait: no se encontró el hilo a bloquear");
}
void suseSignal(char* nombreSemaforo) {
	t_semaforo* unSemaforo = buscarSemaforo(nombreSemaforo);

	if(unSemaforo->valor < unSemaforo->valorMaximo)
		unSemaforo->valor ++;

	if(!queue_is_empty(unSemaforo->colaBloqueo)) {
		pthread_mutex_lock(&mutexListaSemaforos);
		t_hilo* unHilo = queue_pop(unSemaforo->colaBloqueo);
		pthread_mutex_unlock(&mutexListaSemaforos);
		desbloquearHilo(unHilo);

	}
}
