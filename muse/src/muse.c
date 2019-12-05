#include <stdio.h>
#include <stdlib.h>
#include "muse.h"

int main(void) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}

void inicializarLogger(char* path){//0 es archivo, 1 es consola
	char* nombre = string_new();
	string_append(&nombre,path);
	string_append(&nombre,".log");
	logger = log_create(nombre,"muse",1,LOG_LEVEL_TRACE);
	free(nombre);
}

void cargarConfiguracion(){
	t_config* config = config_create("muse.config");
	PUERTO = config_get_int_value(config, "LISTEN_PORT");
	TAMANIO_MEMORIA = config_get_int_value(config, "MEMORY_SIZE");
	TAMANIO_PAGINA = config_get_int_value(config, "PAGE_SIZE");
	TAMANIO_SWAP = config_get_int_value(config, "SWAP_SIZE");
}

void inicializarEstructuras(){
	posicionInicialMemoria = malloc(TAMANIO_MEMORIA);
	espacioDisponible = TAMANIO_MEMORIA + TAMANIO_SWAP;
	CANT_PAGINAS_MEMORIA = TAMANIO_MEMORIA / TAMANIO_PAGINA;
	CANT_PAGINAS_SWAP = TAMANIO_SWAP / TAMANIO_PAGINA;
	listaMapeos = list_create();
	listaProgramas = list_create();
	punteroClock = 0;
	inicializarMemoriaVirtual("unaruta");
	inicializarBitmap();
	inicializarSemaforos();
}

void inicializarBitmap(){
	bitmap = malloc(sizeof(Bitmap));
	bitmap->tamanio_memoria = TAMANIO_MEMORIA;
	bitmap->bits_memoria = list_create();
	for(uint32_t i = 0; i < TAMANIO_MEMORIA; i++){
		BitMemoria* bit = malloc(sizeof(BitMemoria));
		bit->esta_ocupado = false;
		bit->pos = i;
		bit->bit_modificado = false;
		bit->bit_uso = false;
		list_add(bitmap->bits_memoria,bit);
	}

	bitmap->tamanio_memoria_virtual = TAMANIO_SWAP;
	bitmap->bits_memoria_virtual = list_create();
	for(uint32_t i = 0; i < TAMANIO_SWAP; i++){
		BitSwap* bit = malloc(sizeof(BitSwap));
		bit->esta_ocupado = false;
		bit->pos = i;
		list_add(bitmap->bits_memoria_virtual,bit);
	}

}

void inicializarMemoriaVirtual(char* rutaSwap){
	int i = strlen(rutaSwap);
	for(;i>=0;i--)
	{
		if (rutaSwap[i]=='/')
		{
			break;
		}
	}
	char* aux = string_substring_until(rutaSwap,i);
	free(rutaSwap);
	rutaSwap = string_new();
	string_append(&rutaSwap,aux);
	string_append(&rutaSwap,"/AreaSwap");
	log_info(logger,"Ruta area Swap = %s",rutaSwap);
	free(aux);

	fileDescriptor = open(rutaSwap,O_RDWR|O_CREAT,0777);
	if(fileDescriptor<0){
		log_info(logger,"F en el chat por el archivo de swap que no se pudo abrir");
	}
	ftruncate(fileDescriptor,0);
	ftruncate(fileDescriptor,TAMANIO_SWAP);
	posicionInicialSwap = mmap(NULL, TAMANIO_SWAP, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fileDescriptor, 0);
	if(posicionInicialSwap == MAP_FAILED || posicionInicialSwap == NULL){
		perror("error: ");
	}

}

void inicializarSemaforos(){
	pthread_mutex_init(&mut_espacioDisponible,NULL);
	pthread_mutex_init(&mut_listaProgramas,NULL);
	pthread_mutex_init(&mut_bitmap,NULL);
}

int muse_map(char* id, char* path, uint32_t length, uint32_t flag){
	int tamanioAMapear = techo(length) * TAMANIO_PAGINA;
	pthread_mutex_lock(&mut_espacioDisponible);
	if(espacioDisponible >= tamanioAMapear){
		espacioDisponible -= tamanioAMapear;
		pthread_mutex_unlock(&mut_espacioDisponible);
		t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
		if(listaSegmentos == NULL){
			log_info(logger,"F por el programa %s que no existe",id);
			return -1;
		}
		Segmento* segmentoNuevo = malloc(sizeof(Segmento));
		segmentoNuevo->num_segmento = listaSegmentos->elements_count;
		segmentoNuevo->es_mmap = true;
		segmentoNuevo->tamanio_mapeo = length;
		segmentoNuevo->base_logica = base_logica_segmento_nuevo(listaSegmentos);
		//segmentoNuevo->info_heaps = NULL;
		//segmentoNuevo->paginas_liberadas=0;
		segmentoNuevo->path_mapeo = string_duplicate(path);
		segmentoNuevo->tamanio = tamanioAMapear;
		if(flag != MAP_PRIVATE){
			segmentoNuevo->compartido = true;
			t_list* paginas = obtenerMapeoExistente(path,length);
			if(paginas != NULL){
				segmentoNuevo->paginas = paginas;
				list_add(listaSegmentos,segmentoNuevo);
				return segmentoNuevo->base_logica;
			}
		}
		else{
			segmentoNuevo->compartido = false;
		}
		//tengo que crear el el mapeo de cero y llenar lista de mapeo
		t_list* paginas = list_create();
		int cantidad_de_paginas = techo(length);
		for(int i = 0; i < cantidad_de_paginas; i++){
			Pagina* pag = malloc(sizeof(Pagina));
			pag->num_pagina = i;
			pag->bit_marco = NULL;
			pag->presencia = false;
			pag->bit_swap = NULL;
			list_add(paginas,pag);
		}
		segmentoNuevo->paginas = paginas;
		list_add(listaSegmentos,segmentoNuevo);

		Mapeo* mapeo = malloc(sizeof(Mapeo));
		mapeo->contador = 1;
		mapeo->paginas = paginas;
		mapeo->path = string_duplicate(path);
		mapeo->tamanio = length;
		//mapeo->tamanio_de_pags = segmentoNuevo->tamanio;
		pthread_mutex_lock(&mut_mapeos);
		list_add(listaMapeos,mapeo);
		pthread_mutex_unlock(&mut_mapeos);
		return segmentoNuevo->base_logica;
	}
	else{
		pthread_mutex_unlock(&mut_espacioDisponible);
		//no hay lugar
		return -1;
	}
}

int muse_sync(char* id, uint32_t addr, size_t len){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL || list_is_empty(listaSegmentos)){
		log_info(logger,"F por el programa %s que no existe o no tiene segmentos",id);
		return -1;
	}

	Segmento* segmentoEncontrado = segmentoAlQuePertenece(listaSegmentos,addr);
	if(segmentoEncontrado == NULL){
		log_info(logger,"F por el programa %s que no encontro el segmento",id);
		return -1;
	}
	int dirAlSegmento = addr - segmentoEncontrado->base_logica;
		if(segmentoEncontrado->es_mmap){
			int primerPag = dirAlSegmento / TAMANIO_PAGINA;
			int ultimaPag = techo((dirAlSegmento + len) / TAMANIO_PAGINA) - 1;
			int cantidadPags = ultimaPag - primerPag + 1;
			int tamSync = cantidadPags * TAMANIO_PAGINA;

			void* auxiliar = malloc(tamSync);
			int puntero = 0;

			paginasMapEnMemoria(dirAlSegmento,len,segmentoEncontrado);

			int i = primerPag;
			while(i <= ultimaPag){
				Pagina* pag = list_get(segmentoEncontrado->paginas,i);
				void* punteroMarco = obtenerPunteroAMarco(pag);
				memcpy(auxiliar+puntero,punteroMarco,TAMANIO_PAGINA);
				i++;
				puntero += TAMANIO_PAGINA;
				tamSync -= TAMANIO_PAGINA;
			}
			int posInicial = primerPag * TAMANIO_PAGINA;
			FILE* file = fopen(segmentoEncontrado->path_mapeo,"r+");
			if(file != NULL){
				if(fseek(file,posInicial,SEEK_SET == 0)){
					fwrite(auxiliar,cantidadPags*TAMANIO_PAGINA,1,file);
					free(auxiliar);
					fclose(file);
					return 0;
				}
			}
			free(auxiliar);
		}
		log_info(logger,"F porque el segmento no es mmap",id); // si llega aca me suicido
		return -1;
}

int muse_unmap(char* id, uint32_t dir){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL){
		log_info(logger,"F por el programa %s, no se encontró su lista de segmentos", id);
		return -1;
	}
	Segmento* segmentoEncontrado = segmentoAlQuePertenece(listaSegmentos,dir);
	if(segmentoEncontrado != NULL){
		if(segmentoEncontrado->es_mmap){
			unmapear(segmentoEncontrado,id);
			return 0;
		}
	}
	perror("FFFFFFFF");
	return -1;
}


