#include <stdio.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include "libmuse.h"


//cosas que van en muse
int puerto;
int tam_memoria;
int tam_pagina;
int tam_swap;
int posicion_inicial_mem;
int posicion_final_mem;

t_bitarray* bitmap;
void init_bitmap();
void mostrar_bitmap();
void imprimir_info_segmento(struct Segmento *segmento,int index);
void imprimir_info_paginas_segmento(struct Segmento *segmento,int index);
int buscarFrame();
int cantidadFramesDisponibles();
int realizar_primer_asignacion(int frames_necesarios, uint32_t tam);
int aniadir_segmento(int frames_necesarios,int tam);
void crear_paginas(struct Segmento *segmento, int frames_necesarios, uint32_t tam);
void asignar_en_frame(uint32_t tam, int frame);
int segmento_con_lugar(int tam);
void mostrar_frames();
void* memory;
struct HeapMetadata *bigMemory;


//fijarse si se pueden utilizar parametros globales en libmuse
void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar);
int cant_frames;
t_list * lista_segmentos = NULL;

//TODO LO DEL MAIN VA EN EL PROGRAMA QUE LO EJECUTA EXCEPTO LOS CONFIGS Y LO BITMAP.
int main(){
	cargar_configuracion();
	initialize();
	struct HeapMetadata *actual = (struct HeapMetadata *)memory;
	init_bitmap();
	printf("Marcos creados: %d\n", cantidadFramesDisponibles());
	uint32_t p = muse_alloc(40);
	divider();
	mostrar_frames();
	divider();
	uint32_t o = muse_alloc(5);
	divider();
	uint32_t z = muse_alloc(20);
//	actual++;
//	printf("tamanio: %zu\n",actual->tamanio);
	//actual = bigMemory;// esto se tiene que ir
	//uint32_t p;
	//p = muse_alloc(70);
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
	struct HeapMetadata *actual;// va en MUSE
	struct Segmento *segmento;
	uint32_t result;
	int frame_obtenido;
	int frames_necesarios = calcular_frames_necesarios(tam);
	if(!tam){
		printf("No se ha solicitado memoria\n");
		return 0;
	}
	if(cantidadFramesDisponibles() < frames_necesarios){
		printf("no hay memoria disponible");
		return NULL; //ERROR NO HAY MEMORIA DISPONIBLE
	}
	puts("Aniadiendo segmento...");
	int seg = aniadir_segmento(frames_necesarios,tam);
	segmento = list_get(lista_segmentos, seg);
	imprimir_info_segmento(segmento,seg);
	imprimir_info_paginas_segmento(segmento,seg);

}

int segmento_con_lugar(int tam){
	struct Segmento* segmento;
	struct Pagina* pagina;
	struct HeapMetadata* hmetadata;
	int queda = tam_pagina - 5;
	for(int i = 0;i <= list_size(lista_segmentos);i++){
		segmento = list_get(lista_segmentos, i);
		if(segmento->es_mmap == 0){//si no es un mmap el segmento, falta ver como hacer la condicion
		pagina = list_get(segmento->tabla_de_paginas, 0);
		hmetadata = (((int)memory + (pagina->numero_frame * tam_pagina)));
		int recorrido = 0;
		int aux;
		do{
			if(hmetadata->libre == 1){ // si esta libre
				return (int)hmetadata;
			}
			else{
				if((hmetadata->tamanio + 5) > queda){
					queda += tam_pagina;
					queda -= (hmetadata->tamanio + 5);
					aux = tam_pagina - queda;
					recorrido++;
					pagina = list_get(segmento->tabla_de_paginas, recorrido);
					hmetadata = (int)memory + (pagina->numero_frame * tam_pagina) + aux;
				}
				else{
					queda -= (hmetadata->tamanio + 5);
					hmetadata += (hmetadata->tamanio + 5);
				}
			}
		}while(list_size(segmento->tabla_de_paginas) >= recorrido);
		return (int)hmetadata;
		}
	}
	return -1; // funcion que devuelve el indice de la tabla de segmentos en el cual se podria alocar
}
int aniadir_segmento(int frames_necesarios, int tam){
	int num_segmento_a_insertar;
	int segmento_disponible;
	struct Segmento* ultimoTabla;
	struct Segmento* segmento = malloc(sizeof(*segmento));
	struct HeapMetadata* posicion_metadata;
	if(lista_segmentos == NULL){
		num_segmento_a_insertar = realizar_primer_asignacion(frames_necesarios,tam);
		return num_segmento_a_insertar;
	}
	segmento_disponible = segmento_con_lugar(tam);
	if(segmento_disponible != -1) { // algun segmento tiene lugar disponible
	 // segmento disponible tiene la posicion de la metadata donde se puede hacer el split
		posicion_metadata = segmento_disponible;
		printf("Hay segmentos disponibles \n");


		return segmento_disponible;
	}else if(ultimoTabla->es_mmap){ //crear uno nuevo(caso de que ningun segmento tenga lugar disponible, y no se puede agrandar debido a un map)
		printf("no hay segmentos disponibles, se crea uno nuevo \n");
		ultimoTabla = list_get(lista_segmentos, list_size(lista_segmentos)-1);
		segmento->comienzo = ultimoTabla->fin + 1;
		segmento->fin = segmento->comienzo + (frames_necesarios * tam_pagina) - 1;
		segmento->es_mmap = 0;
		crear_paginas(segmento,frames_necesarios,tam);// <-- esta es la funcion que deberia crear las paginas, o sea hacer lo de acá arriba
		num_segmento_a_insertar = list_add(lista_segmentos,segmento);
		imprimir_info_paginas_segmento(&segmento,num_segmento_a_insertar);
		return num_segmento_a_insertar;

	} else {// este caso ocurre cuando no entra en ninguno de los segmentos Y el último no es mmap (puedo y tengo que extender)
		//extender último segmento
	}
}

int realizar_primer_asignacion(int frames_necesarios, uint32_t tam){
	struct Segmento* segmento = malloc(sizeof(*segmento));
	int num_segmento_a_insertar;
	int frame_obtenido;
	int primer_frame;

	lista_segmentos = list_create(lista_segmentos);
	segmento->comienzo = 0;
	segmento->fin = (frames_necesarios * tam_pagina) - 1;
	segmento->es_mmap = 0;

	t_list* lista_paginas;
	lista_paginas = list_create();
	segmento->tabla_de_paginas = lista_paginas;
	for(int i = 1; i <= frames_necesarios ; i++){
		frame_obtenido = buscarFrame();
		if(i == 1){
			primer_frame = frame_obtenido;
		}
		struct Pagina* pagina = malloc(sizeof(*pagina));
		pagina->bit_presencia = 1;
		pagina->numero_frame = frame_obtenido;
		int num_pagina_a_insertar = list_add((lista_paginas),pagina);
	}

	struct HeapMetadata *actual = posicion_inicial_mem + primer_frame * tam_pagina;

	actual->libre = 0;
	actual->tamanio = tam;

	actual += ((actual->tamanio) / sizeof(struct HeapMetadata)) + 1;

	actual->libre = 1;
	if(frames_necesarios == 1){
		actual->tamanio = tam_pagina - tam - sizeof(struct HeapMetadata) * 2;
	} else {
		actual->tamanio = 2 * tam_pagina - sizeof(struct HeapMetadata) * 2 - tam;
	}

	num_segmento_a_insertar = list_add(lista_segmentos,segmento);
	return num_segmento_a_insertar;
}

void crear_paginas(struct Segmento *segmento, int frames_necesarios, uint32_t tam){
	t_list* lista_paginas;
	int frame_obtenido;
	lista_paginas = list_create();
	segmento->tabla_de_paginas = lista_paginas;
	for(int i = 1; i <= frames_necesarios ; i++){
		frame_obtenido = buscarFrame();
		asignar_en_frame(tam,frame_obtenido);
		struct Pagina* pagina = malloc(sizeof(*pagina));
		pagina->bit_presencia = 1;
		pagina->numero_frame = frame_obtenido;
		int num_pagina_a_insertar = list_add((lista_paginas),pagina);
	}
}

void asignar_en_frame(uint32_t tam, int frame){ //checkear para asignar las metadatas
	struct HeapMetadata *actual = posicion_inicial_mem + frame * tam_pagina;

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
void initialize(){
	memory = malloc(tam_memoria);
	cant_frames = tam_memoria / tam_pagina;
	posicion_inicial_mem = memory;
	posicion_final_mem = memory + tam_memoria;
	printf("Direccion inicial de memoria: %zu\n", posicion_inicial_mem);
	printf("Direccion final   de memoria: %zu\n", posicion_final_mem);
}


//todo esto va en muse

void mostrar_frames(){
	struct HeapMetadata* actual = memory;
	while((void*)actual < posicion_final_mem){
		imprimir_direccion_puntero(actual,"actual");
		if(actual->libre == 0 || actual->libre == 1){
			imprimir_direccion_puntero(actual,"actual ocupado");
			printf("... y su tamaño es: %zu\n",actual->tamanio);
			actual += ((actual->tamanio) / sizeof(struct HeapMetadata));
		}
		actual++;
	}
}

int buscarFrame(){
	for(int i = 0; i < cant_frames; i++){
		if(bitarray_test_bit(bitmap,i) == 0){
			bitarray_set_bit(bitmap, i);
			return i;
		}
	}
	return -1; //CASO ERROR
}
int cantidadFramesDisponibles(){
	int contador = 0;
	for(int i = 0; i < cant_frames; i++){
			if(bitarray_test_bit(bitmap,i) == 0){
				contador++;
			}
	}
	return contador;
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

void init_bitmap(){
	bitmap = bitarray_create_with_mode(memory, cant_frames, LSB_FIRST);
	for(int i = 0; i < cant_frames; i++){
		bitarray_clean_bit(bitmap, i);
	}
}

void mostrar_bitmap(){
	for(int i = 0; i < cant_frames; i++){
		printf("El valor del bit (frame) en la posicion %d es: %d\n", i, bitarray_test_bit(bitmap, i));
	}
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

void imprimir_info_segmento(struct Segmento *segmento,int index){
	printf("      Índice segmento: %d\n",index);
	printf("Comienzo del segmento: %d\n",((struct Segmento*)list_get(lista_segmentos,index))->comienzo);
	printf("     Fin del segmento: %d\n",((struct Segmento*)list_get(lista_segmentos,index))->fin);
}

void imprimir_info_paginas_segmento(struct Segmento *segmento,int index){
	int cantidad_paginas_segmento = list_size(((struct Segmento*)list_get(lista_segmentos,index))->tabla_de_paginas);
	for(int i = 0; i < cantidad_paginas_segmento ; i++){
		printf("Bit presencia segmento %d, página %d: %d\n",index, i,((struct Pagina*)list_get(((struct Segmento*)list_get(lista_segmentos,index))->tabla_de_paginas,i))->bit_presencia);
		printf(" Número frame segmento %d, página %d: %d\n",index, i,((struct Pagina*)list_get(((struct Segmento*)list_get(lista_segmentos,index))->tabla_de_paginas,i))->numero_frame);
	}
}
