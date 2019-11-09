#include <stdio.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include "libmuse.h"


//cosas que van en muse
int puerto;
int tam_memoria;
int tam_pagina;
int tam_swap;
t_bitarray* bitmap;
void init_bitmap();
void mostrar_bitmap();
void* memory;
struct HeapMetadata *bigMemory;



void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar);
int cant_frames;

//TODO LO DEL MAIN VA EN EL PROGRAMA QUE LO EJECUTA EXCEPTO LOS CONFIGS Y LO BITMAP.
int main(){
	cargar_configuracion();
	initialize();
	init_bitmap();
	uint32_t p;
	p = muse_alloc(70);
	printf("Allocation and deallocation is done successfully!");
	return 0;

}

int calcular_frames_necesarios(uint32_t tam){

	int frames_necesarios = 0;
	float aux = ((float)tam / (float)tam_pagina);
	do{
		frames_necesarios++;
	}while(aux > frames_necesarios);
	return frames_necesarios;
}

uint32_t muse_alloc(uint32_t tam){
	struct HeapMetadata *actual;
	uint32_t result;
	int frames_necesarios = calcular_frames_necesarios(tam);

	if(!tam){
		printf("No se ha solicitado memoria\n");
		return 0;
	}

	if(Process id tiene ya un segmento creado){ //VER SI SE PUEDE USAR GETPID() DENTRO DE LA BIBLIOTECA.
		//APUNTAR ACTUAL AL PRINCIPO DEL BLOQUE DE VIRTUAL/REAL DEL SEGMENTO RESPECTIVO
		while((((actual->tamanio) < tam) || ((actual->libre) == 0)) && ((void*)actual <= (POSICION FINAL MEMORIA VIRTUAL /FRAME))){ //VER
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
	}

/*	actual = bigMemory;// esto se tiene que ir
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
}

void muse_free(uint32_t dir){
//	printf("Direccion a liberar: %zu\n", dir);
	if((memory <= dir) && (dir <= (memory+tam_memoria))){ //ACA EN LUGAR DE MEMORY DEVERIA IR LA DIRECCION DE VIRTUAL + EL TAMAÑO DEL SEGMENTO
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
	//imprimir_direccion_puntero(actual,"Actual");
	//imprimir_direccion_puntero(siguiente,"Siguiente");
	//printf("Siguiente se corrió: %d\n",((void*)siguiente - (void*)actual));
	while(((void*)siguiente) < (memory+tam_memoria)){ //mientras que no se vaya a la mierda de la memoria (revisar)
		if((actual->libre) && (siguiente->libre)){
			//printf("Mergeando direccion actual %u con siguiente %u\n", ((void*)actual),((void*)siguiente));
			//printf("actual->tamanio antes del merge: %zu\n",actual->tamanio);
			actual->tamanio += (siguiente->tamanio) + sizeof(struct HeapMetadata);
			siguiente += 1 + ((siguiente->tamanio) / sizeof(siguiente));
			//printf("actual->tamanio después del merge: %zu\n",actual->tamanio);
		}
		siguiente += 1 + ((actual->tamanio) / sizeof(siguiente));
	}
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

void init_bitmap(){
	bitmap = bitarray_create_with_mode(memory, cant_frames, LSB_FIRST);
	printf("%d\n", cant_frames);
	for(int i = 0; i < cant_frames; i++){
		bitarray_clean_bit(bitmap, i);
	//	printf("El valor del bit (frame) en la posicion %d es: %d\n", i, bitarray_test_bit(bitmap, i));
	}
}
void mostrar_bitmap(){
	for(int i = 0; i < cant_frames; i++){
		printf("El valor del bit (frame) en la posicion %d es: %d\n", i, bitarray_test_bit(bitmap, i));
	}
}
void initialize(){
	memory = malloc(tam_memoria);
	bigMemory = memory;
	bigMemory->tamanio = tam_memoria - sizeof(struct HeapMetadata);
	bigMemory->libre = 1;
	cant_frames = tam_memoria / tam_pagina;
	printf("Memoria libre: %zu\n",bigMemory->tamanio);
//	printf("Direccion inicial de memoria: %zu\n", memory);
//	printf("Direccion final   de memoria: %zu\n", (memory + tam_memoria));
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
