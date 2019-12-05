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
