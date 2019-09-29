#include <stdio.h>
#include "libmuse.h"

void initialize();
void merge(); // no la usamos

void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar);
//int main(){
//	initialize();
//	uint32_t prueba = muse_alloc(19988);
//	printf("Devuelve %zu",prueba);
//}

int main(){
	initialize();
	uint32_t p = muse_alloc(4900*sizeof(int));
	muse_free(p);
	uint32_t q = muse_alloc(4900*sizeof(int));
	muse_free(q);
	uint32_t r = muse_alloc(1000*sizeof(int));
	uint32_t k = muse_alloc(500*sizeof(int));
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
	struct HeapMetadata *actual,*anterior;
	uint32_t result;
	if(!tam){
		printf("No se ha solicitado memoria\n");
		return 0;
	}
	actual = bigMemory;
	while((((actual->tamanio) < tam) || ((actual->libre) == 0)) && (actual->tamanio != 0)){
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
		printf("Puntero: %p\n", ((void*)actual));
		result = (uint32_t)(++actual);
		printf("Se alocó un bloque de tamanio %zu haciendo un split\n",tam);
		return result;
	} else {
		result = NULL;
		printf("No hay suficiente espacio para alocar la memoria de tamanio %zu\n",tam);
		return result;
	}
}

void muse_free(uint32_t dir){


	if(((void*)bigMemory <= dir) && (dir <= (void*)(bigMemory+20000))){
		struct HeapMetadata *actual = dir;
		actual -= 1;
		actual->libre=1;
		printf("Memoria liberada exitosamente \n");
		//merge(); por ahora no se usa
	}
	else
	{
		printf("Please provide a valid pointer allocated by MyMalloc\n");
	}


}
void merge(){ //no la usamos HAY QUE CHECKEAR
	struct HeapMetadata *actual,*siguiente;
	uint32_t acumulador = 0;
	actual = bigMemory;
	//siguiente = NULL;
	//siguiente = actual + (actual->tamanio) + sizeof(struct HeapMetadata);
	while(acumulador < 20000)
	{
		if((actual->libre)/* && (siguiente->libre)*/){
			actual->tamanio += /*(siguiente->tamanio)*/ sizeof(struct HeapMetadata);
			//siguiente = siguiente + (siguiente->tamanio)+ sizeof(struct HeapMetadata);
			acumulador += actual->tamanio;
		}
		//actual = siguiente;
	}
}

void initialize(){
	bigMemory->tamanio = 20000 - sizeof(struct HeapMetadata);
	printf("Memoria libre: %zu\n",bigMemory->tamanio);
	bigMemory->libre = 1;
}

void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar){
	struct HeapMetadata *new = (void*)((void*)fitting_slot + tamanioAAlocar + sizeof(struct HeapMetadata));
	new->tamanio = (fitting_slot->tamanio) - tamanioAAlocar - sizeof(struct HeapMetadata);
	new->libre = 1;
	fitting_slot->tamanio = tamanioAAlocar;
	fitting_slot->libre = 0;
}
