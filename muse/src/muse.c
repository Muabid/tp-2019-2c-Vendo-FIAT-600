#include <stdio.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
//#include <shared/net.h>
//#include <shared/protocol.h>
//#include <shared/utils.h>
/* #include "net.h"
#include "protocol.h"
#include "utils.h" */ //quiero incluir las cosas de sockets que estan en las queridas commons.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
int puerto;
int tam_memoria;
int tam_pagina;
int tam_swap;
t_bitarray* bitmap;
//void cargar_configuracion();

/*
int main(){  //no se puede poner el main por que se esta ejecutando en libmuse
	cargar_configuracion();
	int socket_servidor = init_server(puerto);
	return 0;
}

void cargar_configuracion(){
	t_config* config = config_create("muse.config");
	puerto = config_get_int_value(config, "LISTEN_PORT");
	tam_memoria = config_get_int_value(config, "MEMORY_SIZE");
	tam_pagina = config_get_int_value(config, "PAGE_SIZE");
	tam_swap = config_get_int_value(config, "SWAP_SIZE");
//	printf("Puerto: %d\nTamanio Memoria: %d\nTamanio Pagina: %d\nTamanio SWAP: %d\n",puerto,tam_memoria,tam_pagina,tam_swap);
}
*/
/*		while((((actual->tamanio) < tam) || ((actual->libre) == 0)) && ((void*)actual <= (POSICION FINAL MEMORIA VIRTUAL /FRAME))){ //VER
				actual += ((actual->tamanio) / sizeof(struct HeapMetadata)) + 1;
		}
		if((actual->tamanio) == tam){
			actual->libre = 0;
			result = (uint32_t)(++actual);
			printf("Se alocó un bloque de tamanio %zu perfectamente\n",tam);
			return result;
		} else if((actual->tamanio) > tam){
			split(actual,tam);
			result = (uint32_t)(++actual);
			printf("Se alocó un bloque de tamanio %zu haciendo un split\n",tam);
			return result;
		} else {
			printf("No hay suficiente espacio para alocar la memoria de tamanio %zu\n",tam);
			result = SIGSEGV;
			return result; //Segmentation fault
		}
	}else{
		lista_segmentos = list_create();
	}
	actual = bigMemory;// esto se tiene que ir
	while((((actual->tamanio) < tam) || ((actual->libre) == 0)) && ((void*)actual <= (memory+tam_memoria))){
		actual += ((actual->tamanio) / sizeof(struct HeapMetadata)) + 1;
	}
	if((actual->tamanio) == tam){
		actual->libre = 0;
		result = (uint32_t)(++actual);
		printf("Se alocó un bloque de tamanio %zu perfectamente\n",tam);
		return result;
	} else if((actual->tamanio) > tam){
		split(actual,tam);
//		printf("El puntero a metadata se encuentra en: %u\n", ((void*)actual));
		result = (uint32_t)(++actual);
//		printf("El puntero a     data se encuentra en: %u\n", ((void*)actual));
		printf("Se alocó un bloque de tamanio %zu haciendo un split\n",tam);
		return result;
	} else {
		result = NULL;
		printf("No hay suficiente espacio para alocar la memoria de tamanio %zu\n",tam);
		return result;
	}
*/
