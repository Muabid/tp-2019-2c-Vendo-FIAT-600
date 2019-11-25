#include <stdio.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include "libmuse.h"


//cosas para Muse
void* memory;
struct HeapMetadata *bigMemory;
uint32_t aniadir_segmento(int frames_necesarios,int tam);
uint32_t realizar_primer_asignacion(uint32_t tam);
uint32_t asignar_en_frame(uint32_t tam, struct Segmento* segmento,int framesAgregados);
int segmento_con_lugar(int tam);
int buscarFrame();
void init_bitmap();
void mostrar_bitmap();
void mostrar_frames();
void imprimir_info_segmento(struct Segmento *segmento,int index);
void imprimir_info_paginas_segmento(struct Segmento *segmento,int index);
int cantidadFramesDisponibles();
void crear_paginas(struct Segmento *segmento, int frames_necesarios);
void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar, int restante,struct Segmento *segmento, int indice_pagina);
void cargar_configuracion();
void initialize();
   //void merge();
void imprimir_direccion_puntero(struct HeapMetadata *ptr, char nombre_ptr[]);
void divider();
int calcular_frames_necesarios(uint32_t tam);

t_bitarray* bitmap;
int puerto;
int tam_memoria;
int tam_pagina;
int tam_swap;
int posicion_inicial_mem;
int posicion_final_mem;


//fijarse si se pueden utilizar parametros globales en libmuse
int cant_frames;
t_list * lista_segmentos = NULL;





//TODO LO DEL MAIN VA EN EL PROGRAMA QUE LO EJECUTA EXCEPTO LOS CONFIGS Y LO BITMAP.
int main(){
	cargar_configuracion();
	initialize();
	init_bitmap();
	printf("Marcos creados: %d\n", cantidadFramesDisponibles());
	uint32_t p = muse_alloc(40);
	uint32_t o = muse_alloc(5);
	uint32_t z = muse_alloc(20);
	//mostrar_bitmap();
	mostrar_frames();
	printf("Allocation and deallocation is done successfully!");
	return 0;

}

void mostrar_frames(){
	struct HeapMetadata* actual = memory;
	int recorrido = 1;
	while(recorrido < 2 ){
		imprimir_direccion_puntero(actual,"actual");
		if(actual->libre == 0 || actual->libre == 1){
			imprimir_direccion_puntero(actual,"actual ocupado");
			printf("... y su tamaño es: %zu\n",actual->tamanio);
			actual += ((actual->tamanio) / sizeof(struct HeapMetadata));
		}
		recorrido++;
		actual++;
	}
}

uint32_t muse_alloc(uint32_t tam){ //CHEQUEDA / NO PROBADA
	uint32_t result;
	int frames_necesarios = calcular_frames_necesarios(tam);
	if(!tam){
		printf("No se ha solicitado memoria\n");
		return 0;
	}
	if(cantidadFramesDisponibles() < frames_necesarios){
		printf("no hay memoria disponible\n");
		return NULL; //ERROR NO HAY MEMORIA DISPONIBLE
	}
	result = aniadir_segmento(frames_necesarios,tam);
	return result;
}

uint32_t aniadir_segmento(int frames_necesarios, int tam){
	int num_segmento_a_insertar, segmento_disponible, aux;
	struct Segmento* ultimoTabla; //se usa como parametro en algunos casos, para no declarar otro puntero nomas
	struct Segmento* segmento = malloc(sizeof(*segmento));
	struct Pagina* pagina;
	struct HeapMetada* fitting_slot;
	uint32_t result;

	//OBTENER LISTA DE SEGMENTOS , POR AHORA LA TRATAMOS COMO UN GLOBAL

	if(lista_segmentos == NULL){

		result = realizar_primer_asignacion(tam);
		return result;
	}

	segmento_disponible = segmento_con_lugar(tam);

	ultimoTabla = list_get(lista_segmentos,((list_size(lista_segmentos)) - 1));

	if(segmento_disponible != -1) { // algun segmento tiene lugar disponible?

		printf("Hay segmentos disponibles \n");
		ultimoTabla = list_get(lista_segmentos,segmento_disponible); //se consigue el segmento con lugar
		result = asignar_en_frame(tam, ultimoTabla, 0);
		return result;

	}else if(ultimoTabla->es_mmap){ //crear uno nuevo(caso de que ningun segmento tenga lugar disponible, y no se puede agrandar debido a un map)

		printf("no hay segmentos disponibles, se crea uno nuevo \n");
		ultimoTabla = list_get(lista_segmentos, list_size(lista_segmentos)-1);
		segmento->comienzo = ultimoTabla->fin + 1;
		segmento->fin = segmento->comienzo + (frames_necesarios * tam_pagina) - 1;
		segmento->es_mmap = 0;
		crear_paginas(segmento,frames_necesarios);
		num_segmento_a_insertar = list_add(lista_segmentos,segmento);
		pagina = list_get(segmento->tabla_de_paginas,0);
		fitting_slot = (pagina -> numero_frame * tam_pagina) + (int)memory;
		result = (int)fitting_slot;
		split(fitting_slot, tam, 0,segmento,0);
		return result;

	} else {
		// este caso ocurre cuando no entra en ninguno de los segmentos Y el último no es mmap (puedo y tengo que extender)
		//extender último segmento
		aux = tam - ((frames_necesarios - 1) * tam_pagina);
		if(segmento_con_lugar(aux) == (list_size(lista_segmentos) - 1)){ //si se puede safar de asignar un frame mas, osea que un pedazo me entre en lo que tengo
			printf("se agrego %d \n",frames_necesarios - 1);
			crear_paginas(ultimoTabla, (frames_necesarios - 1));
			asignar_en_frame(tam, ultimoTabla, (frames_necesarios - 1));
		}else{
			crear_paginas(ultimoTabla, frames_necesarios);
			printf("se agrego %d \n",frames_necesarios);
			asignar_en_frame(tam, ultimoTabla, frames_necesarios);
		}
	}
}

uint32_t realizar_primer_asignacion(uint32_t tam){ //CHEQUEDA / NO PROBADA
	struct Segmento* segmento = malloc(sizeof(*segmento));
	int num_segmento_a_insertar, frame_obtenido, primer_frame;
	int frames_necesarios = calcular_frames_necesarios(tam+5);
	uint32_t result;
	lista_segmentos = list_create(lista_segmentos); //fijarse como es que se pasa con el diccionario las listas
	segmento->comienzo = 0;
	segmento->fin = (frames_necesarios * tam_pagina) - 1;
	segmento->es_mmap = 0;
	t_list* lista_paginas = list_create();
	segmento->tabla_de_paginas = lista_paginas;


	for(int i = 1; i <= frames_necesarios ; i++){
		frame_obtenido = buscarFrame();
		if(i == 1){
			primer_frame = frame_obtenido;
		}
		struct Pagina* pagina = malloc(sizeof(*pagina));
		pagina->bit_presencia = 1;
		pagina->numero_frame = frame_obtenido;
		int num_pagina_a_insertar = list_add((lista_paginas),pagina); //parametro que ni voy a usar pero lo necesito para hacer el puto list add
	}

	struct HeapMetadata *actual = posicion_inicial_mem + primer_frame * tam_pagina;
	result = (int)actual;
	actual->libre = 0;
	actual->tamanio = tam;

	actual += ((actual->tamanio) / sizeof(struct HeapMetadata)) + 1;
	actual->libre = 1;
	actual->tamanio = (frames_necesarios * tam_pagina) - sizeof(struct HeapMetadata) * 2 - tam;

	num_segmento_a_insertar = list_add(lista_segmentos,segmento); //parametro que ni voy a usar pero lo necesito para hacer el puto list add
	return result;
}

int calcular_frames_necesarios(uint32_t tam){ //CHEQUEDA / NO PROBADA

	int frames_necesarios = 0;
	float aux = ((float)(tam + 5) / (float)tam_pagina);
	do{
		frames_necesarios++;
	}while(aux > frames_necesarios);
	return frames_necesarios;
}

int buscarFrame(){ //CHEQUEDA / NO PROBADA
	struct HeapMetadata* hmetadata = (int)memory;
	for(int i = 0; i < cant_frames; i++){
		if(bitarray_test_bit(bitmap,i) == 0){
			printf("%d antes\n",hmetadata->tamanio);
			bitarray_set_bit(bitmap, i); //esta linea de codigo le agrega 1 al metadata del bloque
			printf("%d despues\n",hmetadata->tamanio);
			return i;
		}
	}
	return -1; //CASO ERROR
}

int segmento_con_lugar(int tam){ //CHEQUEDA / NO PROBADA
	struct Segmento* segmento;
	struct Pagina* pagina;
	struct HeapMetadata* hmetadata;
	int queda = tam_pagina;

	//OBTENER LISTA DE SEGMENTOS , POR AHORA LA TRATAMOS COMO UN GLOBAL

	for(int i = 0 ; i < list_size(lista_segmentos) ; i++){
		segmento = list_get(lista_segmentos, i);
		if(segmento->es_mmap == 0){//si no es un mmap el segmento, falta ver como hacer la condicion
			pagina = list_get(segmento->tabla_de_paginas, 0);
			hmetadata = (((int)memory + (pagina->numero_frame * tam_pagina)));
			int recorrido = 0, aux;
			imprimir_direccion_puntero(hmetadata, "INCIO");
			//IMPRIMIR METADATA AL PRINCIPIO PARA VER DONDE ESTOY PARADO

			do{

				printf("%d DISPONIBLE\n",hmetadata->tamanio);
				if(hmetadata->libre == 1 && (hmetadata->tamanio >= tam + 5)){
					imprimir_direccion_puntero(hmetadata, "ENCONTRE ACA");
					return i;
				}
				else{
					if((hmetadata->tamanio + 5) > queda){
						while((hmetadata->tamanio + 5) > queda){
							queda += tam_pagina;
							recorrido++;
							if(recorrido > (list_size(segmento->tabla_de_paginas)-1)){
								return -1;
							}
						}
						queda -= (hmetadata->tamanio + 5);
						aux = tam_pagina - queda;
						pagina = list_get(segmento->tabla_de_paginas, recorrido);
						hmetadata =((int)memory + (pagina->numero_frame * tam_pagina) + aux) ;
						imprimir_direccion_puntero(hmetadata, "SE DESPLAZO");
					}
					else{
						queda -= (hmetadata->tamanio + 5);
						imprimir_direccion_puntero(hmetadata, "ANTES DEL DESPLAZO");
						hmetadata += (hmetadata->tamanio + 5) / 5;
						imprimir_direccion_puntero(hmetadata, "POST DESPLAZO");
					}
				}
			}while(list_size(segmento->tabla_de_paginas) >= recorrido);
		}
	}
	return -1;
}


uint32_t asignar_en_frame(uint32_t tam, struct Segmento* segmento,int framesAgregados){ //CHEQUEDA / NO PROBADA
	struct Pagina* pagina = list_get(segmento -> tabla_de_paginas, 0);
	struct HeapMetadata *hmetadata = (int)memory + (pagina->numero_frame * tam_pagina);
	int salida = 0, aux, recorrido = 0, parametro_para_funcion = tam_pagina;
	int queda = tam_pagina;
	do{
		if(hmetadata->libre == 1 && (hmetadata->tamanio >= tam + 5)){
			salida = 1;
		}
		else{
			if((hmetadata->tamanio + 5) > queda){
				while((hmetadata->tamanio + 5) > queda || salida == 1){
					queda += tam_pagina;
					recorrido++;
					if(recorrido > list_size(segmento->tabla_de_paginas) - framesAgregados){
						salida == 1;
						recorrido --;
					}
				}
				queda -= (hmetadata->tamanio + 5);
				parametro_para_funcion = queda;
				aux = tam_pagina - queda;
				pagina = list_get(segmento->tabla_de_paginas, recorrido);
				hmetadata =((int)memory + (pagina->numero_frame * tam_pagina) + aux) ;
			}
			else{
				queda -= (hmetadata->tamanio + 5);
				hmetadata += (hmetadata->tamanio + 5) / 5;
				parametro_para_funcion = queda;
			}
		}
	}while(salida == 0);
	split(hmetadata, tam, parametro_para_funcion, segmento, recorrido);
	uint32_t result = (int)hmetadata;
	return result;

}


void crear_paginas(struct Segmento *segmento, int frames_necesarios){ //CHEQUEDA / NO PROBADA
	t_list* lista_paginas;
	int frame_obtenido, num_pagina_a_insertar ;
	lista_paginas = segmento->tabla_de_paginas;
	for(int i = 1; i <= frames_necesarios ; i++){
		frame_obtenido = buscarFrame();
		struct Pagina* pagina = malloc(sizeof(*pagina));
		pagina->bit_presencia = 1;
		pagina->numero_frame = frame_obtenido;
		num_pagina_a_insertar = list_add((lista_paginas),pagina);
	}
}


void initialize(){ //CHEQUEDA / NO PROBADA
	memory = malloc(tam_memoria);
	cant_frames = tam_memoria / tam_pagina;
	posicion_inicial_mem = (int)memory;
	posicion_final_mem = (int)memory + tam_memoria;
	printf("Direccion inicial de memoria: %zu\n", posicion_inicial_mem);
	printf("Direccion final   de memoria: %zu\n", posicion_final_mem);
}


int cantidadFramesDisponibles(){ //CHEQUEDA / NO PROBADA
	int contador = 0;
	for(int i = 0; i < cant_frames; i++){
			if(bitarray_test_bit(bitmap,i) == 0){
				contador++;
			}
	}
	return contador;
}
void init_bitmap(){ //CHEQUEDA / NO PROBADA
	bitmap = bitarray_create_with_mode(memory, cant_frames/8, LSB_FIRST); //pasar en bytes
	for(int i = 0; i < cant_frames; i++){
		bitarray_clean_bit(bitmap, i);
	}
}

void mostrar_bitmap(){ //CHEQUEDA / NO PROBADA
	for(int i = 0; i < cant_frames; i++){
		printf("El valor del bit (frame) en la posicion %d es: %d\n", i, bitarray_test_bit(bitmap, i));
	}
}

void split(struct HeapMetadata *fitting_slot, uint32_t tamanioAAlocar, int restante,struct Segmento *segmento, int indice_pagina){ //CHEQUEDA / NO PROBADA
	int salida = 0,acumulado = restante, primera = 0;
	struct Pagina *pagina;
	struct HeapMetadata *new = (void*)((void*)fitting_slot + tamanioAAlocar + sizeof(struct HeapMetadata));
	do{
		if(acumulado < tamanioAAlocar + 5){
			primera = 1;
			acumulado += tam_pagina ;
			indice_pagina++; //esto no funciona si los frames son no contiguos TP DE MIERDA
			pagina = list_get(segmento->tabla_de_paginas,indice_pagina);
			new = (int)memory + (tam_pagina * pagina->numero_frame + 1); //aca sigue rompiendo
		}else if(primera == 0){
			salida = 1;
		}else{
			acumulado -= tamanioAAlocar + 5;
			new -= (acumulado / 5);
			salida = 1;
		}
	}while(salida != 1);
	new->tamanio = acumulado;
	new->libre = 1;
	fitting_slot->tamanio = tamanioAAlocar;
	fitting_slot->libre = 0;
}

void cargar_configuracion(){ //CHEQUEDA
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
