#include <stdio.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <shared/net.h>
#include <shared/protocol.h>
#include <shared/utils.h>
#include <commons/collections/dictionary.h>
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
t_dictionary* diccionario_procesos = dictionary_create();


void recibir_mensaje(void* sock){
	int socket = (int) sock;
	t_message* message = recv_message(sock);

	switch(message->head){
		case MUSE_ALLOC:{
			uint32_t aux = *((uint32_t*) message->content);
			muse_alloc(socket,aux);
			break;
		}
		case MUSE_FREE:{
			uint32_t aux = *((uint32_t*) message->content);
			muse_free(socket,aux);
			break;
		}
		case MUSE_CPY:{
			void * aux = message->content;
			uint32_t dst;
			memcpy(&dst,aux,sizeof(uint32_t));
			aux += sizeof(uint32_t);
			int n;
			memcpy(&n,aux,sizeof(int));
			aux += sizeof(int);
			void* src = aux;
			muse_cpy(socket,dst,src,n);
			break;
		}
		case MUSE_GET:{
			void * aux = message->content;
			uint32_t src;
			memcpy(&src,aux,sizeof(uint32_t));
			aux += sizeof(uint32_t);
			size_t n;
			memcpy(&n,aux,sizeof(size_t));
			aux += sizeof(size_t);
			void* dst = aux;
			muse_get(socket,dst,src,n);
			break;
		}
		case MUSE_MAP:{
			void * aux = message->content;

			break;
		}
		case MUSE_SYNC:{
			void * aux = message->content;
			uint32_t addr;
			memcpy(&addr,aux,sizeof(uint32_t));
			aux += sizeof(uint32_t);
			size_t len;
			memcpy(&len,aux,sizeof(size_t));
			muse_sync(socket,addr,len);
			break;
		}
		case MUSE_UNMAP:{
			uint32_t aux = *((uint32_t*) message->content);
			muse_unmap(socket,aux);
			break;
		}
	}
}


/*
int main(){  //no se puede poner el main por que se esta ejecutando en libmuse
	cargar_configuracion();
	int socket_servidor = init_server(puerto);
	return 0;
}

v

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
