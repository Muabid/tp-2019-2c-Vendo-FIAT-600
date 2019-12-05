#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdint.h>
#include <sys/mman.h>

typedef struct Programa{
	int id;
	t_list* segmentos;
}Programa;

typedef struct Segmento{
	uint32_t num_segmento;
	uint32_t base_logica;
	uint32_t tamanio;
	t_list* paginas;
	t_list* status_metadata;
	bool es_mmap;
	bool compartido;
	char* path_mapeo;
	uint32_t tamanio_mapeo;
}Segmento;

typedef struct Pagina{
	uint32_t num_pagina;
	bool presencia;
	BitMemoria* bit_marco;
	BitSwap* bit_swap;
}Pagina;

typedef struct Bitmap{
	t_list* bits_memoria;//se llena con t_bit_memoria
	uint32_t tamanio_memoria;
	t_list* bits_memoria_virtual;//se llena con t_bit_swap
	uint32_t tamanio_memoria_virtual;
}Bitmap;

typedef struct BitMemoria{
	bool esta_ocupado;
	uint32_t pos;
	bool bit_uso;
	bool bit_modificado;
}BitMemoria;

typedef struct BitSwap{
	bool esta_ocupado;
	uint32_t pos;
}BitSwap;

typedef struct HeapMetadata{
	uint32_t tamanio;
	bool libre;

}__attribute__((packed)) HeapMetadata;

typedef struct Mapeo{
	char* path;
	uint32_t contador;
	t_list* paginas;
	int tamanio;
	//int tamanio_de_pags;
}Mapeo;

typedef struct InfoHeap{
	int direccion_heap;
	int espacio;
	bool libre;
	int indice;
}__attribute__((packed)) InfoHeap;

#endif /* ESTRUCTURAS_H_ */
