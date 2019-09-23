#include <stdio.h>
#include "libmuse.h"

void initialize();

int main(){
	initialize();
	uint32_t prueba = muse_alloc(19988);
	printf("Devuelve %zu",prueba);
}

uint32_t muse_alloc(uint32_t tam){
	struct HeapMetadata *actual,*anterior;
	uint32_t result;
	if(!tam){
		printf("No se ha solicitado memoria\n");
		return 0;
	}
	/*
	if(!(bigMemory->tamanio)){
		//acá se inicializa la big memory si no se inicializó antes??
	}
	*/
	actual = bigMemory;
	while((((actual->tamanio) < tam) || ((actual->libre) == 0)) && (actual->sig != NULL)){
		anterior = actual;
		actual = actual->sig;
		printf("Un bloque de memoria verificado\n");
	}
	if((actual->tamanio) == tam){
//		printf("Tamanio de ptr 'actual': %zu\n",actual->tamanio);
//		printf("Tamanio del bloque que quiero insertar: %zu\n",tam);
		actual->libre = 0;
		result = (uint32_t)(++actual);
		printf("Se alocó un bloque perfectamente\n");
		return result;
	}
	else if((actual->tamanio) > tam){
		//split(actual,tam);
//		printf("Tamanio de ptr 'actual': %zu\n",actual->tamanio);
//		printf("Tamanio del bloque que quiero insertar: %zu\n",tam);
		result = (uint32_t)(++actual);
		printf("Se alocó un bloque haciendo un split\n");
		return result;
	}
	else {
//		printf("Tamanio de ptr 'actual': %zu\n",actual->tamanio);
//		printf("Tamanio del bloque que quiero insertar: %zu\n",tam);
		result = NULL;
		printf("No hay suficiente espacio para alocar la memoria\n");
		return result;
	}
}

void initialize(){
	bigMemory->tamanio = 20000 - sizeof(struct HeapMetadata);
//	printf("Memoria libre: %zu\n",bigMemory->tamanio);
	bigMemory->libre = 1;
	bigMemory->sig = NULL;
}
