#include <stdio.h>
#include <commons/config.h>
#include "libmuse.h"



void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar);

int main(){
	initialize();
	cargar_configuracion();
	divider();
	uint32_t p = muse_alloc(4900*sizeof(int));
	divider();
	muse_free(p);
	divider();
	uint32_t q = muse_alloc(4900*sizeof(int));
	divider();
	muse_free(q);
	divider();
	uint32_t r = muse_alloc(1000*sizeof(int));
	divider();
	uint32_t k = muse_alloc(500*sizeof(int));
	divider();
	printf("Allocation and deallocation is done successfully!");
	return 0;

}

int muse_get(void* dst, uint32_t src, size_t n) {//revisar
	char *csrc = (char *)src; //casteo ambos a char * para manejar
	char *cdst = (char *)dst;
	for (int i=0; i<n; i++) // barro hasta llegar a n
	{
		cdst[i] = csrc[i]; //copio el array
		//falta ver caso de error
	}

	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n){

	char *csrc = (char *)src; //casteo ambos a char * para manejar
	char *cdst = (char *)dst;
	for (int i=0; i<n; i++) // barro hasta llegar a n
	{
		cdst[i] = csrc[i]; //copio el array
		//falta ver caso de error
	}

	return 0;
}



uint32_t muse_alloc(uint32_t tam){
	struct HeapMetadata *actual;
	uint32_t result;
	if(!tam){
		printf("No se ha solicitado memoria\n");
		return 0;
	}
	actual = bigMemory;
	while((((actual->tamanio) < tam) || ((actual->libre) == 0)) && ((void*)actual <= (void*)(memory+20000))){
		actual += ((actual->tamanio) / sizeof(struct HeapMetadata)) + 1;
		printf("Un bloque de memoria verificado\n");
	}
	if((actual->tamanio) == tam){
		actual->libre = 0;
		result = (uint32_t)(++actual);
		printf("Se alocó un bloque de tamanio %zu perfectamente\n",tam);
		return result;
	} else if((actual->tamanio) > tam){
		split(actual,tam);
		printf("El puntero a metadata se encuentra en: %u\n", ((void*)actual));
		result = (uint32_t)(++actual);
		printf("El puntero a     data se encuentra en: %u\n", ((void*)actual));
		printf("Se alocó un bloque de tamanio %zu haciendo un split\n",tam);
		return result;
	} else {
		result = NULL;
		printf("No hay suficiente espacio para alocar la memoria de tamanio %zu\n",tam);
		return result;
	}
}

void muse_free(uint32_t dir){
	printf("Direccion a liberar: %zu\n", dir);
	if(((void*)memory <= dir) && (dir <= (void*)(memory+20000))){
		struct HeapMetadata *actual = (void*)dir;
		actual -= 1;
		actual->libre = 1;
		printf("Memoria liberada exitosamente \n");
		merge();
	} else {
		printf("La dirección de memoria indicada no está asignada (pasaste cualquier cosa)\n");
	}
}
void merge(){
	struct HeapMetadata *actual,*prev,*siguiente;
	actual = siguiente = bigMemory;
	siguiente += 1 + ((actual->tamanio) / sizeof(struct HeapMetadata));
	imprimir_direccion_puntero(actual,"Actual");
	imprimir_direccion_puntero(siguiente,"Siguiente");
	printf("Siguiente se corrió: %d\n",((void*)siguiente - (void*)actual));
	while(((void*)siguiente) < (void*)(memory+20000)){ //mientras que no se vaya a la mierda de la memoria (revisar)
		if((actual->libre) && (siguiente->libre)){
			printf("Mergeando direccion actual %u con siguiente %u\n", ((void*)actual),((void*)siguiente));
			printf("actual->tamanio antes del merge: %zu\n",actual->tamanio);
			actual->tamanio += (siguiente->tamanio) + sizeof(struct HeapMetadata);
			siguiente += 1 + ((siguiente->tamanio) / sizeof(siguiente));
			printf("actual->tamanio después del merge: %zu\n",actual->tamanio);
		}
		siguiente += 1 + ((actual->tamanio) / sizeof(siguiente));
	}
}//if(((void*)memory<=ptr)&&(ptr<=(void*)(memory+20000))){

void initialize(){
	bigMemory->tamanio = 20000 - sizeof(struct HeapMetadata);
	bigMemory->libre = 1;
	printf("Memoria libre: %zu\n",bigMemory->tamanio);
	printf("Direccion inicial de memoria: %zu\n", (void*)memory);
	printf("Direccion final   de memoria: %zu\n", ((void*)(memory + 20000)));
}

void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar){
	struct HeapMetadata *new = (void*)((void*)fitting_slot + tamanioAAlocar + sizeof(struct HeapMetadata));
	new->tamanio = (fitting_slot->tamanio) - tamanioAAlocar - sizeof(struct HeapMetadata);
	new->libre = 1;
	fitting_slot->tamanio = tamanioAAlocar;
	fitting_slot->libre = 0;
}

void cargar_configuracion(){
	t_config* config = config_create("muse.config");
	puerto = config_get_int_value(config, "LISTEN_PORT");
	tam_memoria = config_get_int_value(config, "MEMORY_SIZE");
	tam_pagina = config_get_int_value(config, "PAGE_SIZE");
	tam_swap = config_get_int_value(config, "SWAP_SIZE");
//	printf("Puerto: %d\nTamanio Memoria: %d\nTamanio Pagina: %d\nTamanio SWAP: %d\n",puerto,tam_memoria,tam_pagina,tam_swap);
}

void imprimir_direccion_puntero(struct HeapMetadata *ptr, char nombre_ptr[]){
	printf("El puntero '%s' se encuentra en %u\n",nombre_ptr,(void*)ptr);
}

void divider(){
	puts("------------------------------------------------------------");
}
