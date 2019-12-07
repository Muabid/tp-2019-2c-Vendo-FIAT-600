#include "estructuras.h"
#include "utils.h"
#include "muse.h"

t_list* obtenerListaSegmentosPorId(char* idPrograma){
	bool esElPrograma(Programa* programa){
		return string_equals_ignore_case(programa->id,idPrograma);
	}
	pthread_mutex_lock(&mut_listaProgramas);
	Programa* programaEncontrado = list_find(listaProgramas,(void*)esElPrograma);
	pthread_mutex_unlock(&mut_listaProgramas);
	if(programaEncontrado == NULL){
		return NULL;
	}
	return programaEncontrado->segmentos;
}

t_list* obtenerMapeoExistente(char* path,int tamanio){
	t_list* paginas = NULL;
	void closure(Mapeo* mapeo){
		if(string_equals_ignore_case(mapeo->path,path) && mapeo->tamanio == tamanio){
			paginas = mapeo->paginas;
			mapeo->contador++;
		}
	}
	pthread_mutex_lock(&mut_mapeos);
	list_iterate(listaMapeos,(void*)closure);
	pthread_mutex_unlock(&mut_mapeos);
	return paginas;
}

Segmento* segmentoAlQuePertenece(t_list* listaSegmentos, uint32_t direccion){
	for(int i = 0; i < list_size(listaSegmentos); i++){
		if(direccion - ((Segmento*)list_get(listaSegmentos,i))->base_logica < ((Segmento*)list_get(listaSegmentos,i))->tamanio){
			return ((Segmento*)list_get(listaSegmentos,i));
		}
	}
	return NULL;
}

void* obtenerPunteroAMarco(Pagina* pag){
	if(!pag->presencia){
		if(pag->bit_swap != NULL){
			void* paginaVictima = malloc(TAMANIO_PAGINA);
			memcpy(paginaVictima, posicionInicialSwap + pag->bit_swap->pos * TAMANIO_PAGINA, TAMANIO_PAGINA);
			//copio lo que esta en swap a un puntero temporal
			//pag->bit_swap->esta_ocupado = false;
			pag->bit_swap = NULL;
			BitMemoria* bit = asignarMarcoNuevo();
			pag->bit_marco = bit;
			pag->presencia = true;
			//pego la pag en mem
			memcpy(posicionInicialMemoria + bit->pos * TAMANIO_PAGINA, paginaVictima, TAMANIO_PAGINA);
		}
		else{
			log_info(logger,"I ain't no hollaback girl");
		}
	}
	pag->bit_marco->bit_uso = true;
	return posicionInicialMemoria + pag->bit_marco->pos * TAMANIO_PAGINA;
}

BitMemoria* asignarMarcoNuevo(){
	BitMemoria* bit = buscarBitLibreMemoria();
	if(bit == NULL){
		bit = ejecutarClockModificado();
	}
	return bit;
}

BitMemoria* buscarBitLibreMemoria(){
	pthread_mutex_lock(&mut_bitmap);
	//si encuentro uno, ya le pongo como que esta usado
	bool bit_libre(BitMemoria* bit){
		return !bit->esta_ocupado;
	}
	BitMemoria* bitEncontrado = list_find(bitmap->bits_memoria,(void*)bit_libre);
	if(bitEncontrado!=NULL){
		bitEncontrado->esta_ocupado = true;
		bitEncontrado->bit_modificado = true;
		bitEncontrado->bit_uso = true;
	}
	pthread_mutex_unlock(&mut_bitmap);
	return bitEncontrado;
}

void paginasMapEnMemoria(int direccion, int tamanio, Segmento* segmentoEncontrado){
	int primerPag = direccion / TAMANIO_PAGINA;
	int offset = direccion % TAMANIO_PAGINA;
	int ultimaPag = techo((direccion+tamanio) / TAMANIO_PAGINA) - 1;

	if(tienePaginasNoCargadasEnMemoria(segmentoEncontrado,primerPag,ultimaPag)){
		int cantidadPags = ultimaPag - primerPag + 1;
		int bytesPorLeer = cantidadPags * TAMANIO_PAGINA;
		int relleno = bytesPorLeer - offset - tamanio;
		void* bloqueRelleno = generarRelleno(relleno);
		int ultimaPaginaLista = segmentoEncontrado->paginas->elements_count-1; // is that allowed, WHAT THE FUCK IS THIS ALLOWED

		void* buffer = malloc(bytesPorLeer);
		FILE* archivo = fopen(segmentoEncontrado->path_mapeo,"rb");
		fread(buffer,TAMANIO_PAGINA,cantidadPags,archivo);
		fclose(archivo);
		int puntero = primerPag * TAMANIO_PAGINA;
		int i = primerPag;
		while(i <= ultimaPag){
			Pagina* pag = (Pagina*)list_get(segmentoEncontrado,i);
			if(pag->bit_marco == NULL && pag->bit_swap == NULL){
				pag->bit_marco = asignarMarcoNuevo();
				pag->bit_marco->bit_uso = true;
				pag->bit_marco->bit_modificado = true;
				pag->presencia = true;

				void* punteroMarco = obtenerPunteroAMarco(pag);
				if(pag->num_pagina == ultimaPaginaLista){
					memcpy(punteroMarco, buffer + puntero, TAMANIO_PAGINA - relleno);
					memcpy(punteroMarco + TAMANIO_PAGINA - relleno, bloqueRelleno,relleno);
				} else {
					memcpy(punteroMarco,buffer + puntero, TAMANIO_PAGINA);
				}
			}

			i++;
			puntero += TAMANIO_PAGINA;
			bytesPorLeer -= TAMANIO_PAGINA;
		}
		free(buffer);
		free(bloqueRelleno);
	}
}

bool tienePaginasNoCargadasEnMemoria(Segmento* segmento, int pagInicial, int pagFinal){
	for(int i = 0; i < list_size(segmento->paginas); i++){
		Pagina* pag = ((Pagina*)list_get(segmento->paginas,i));
		if(pag->num_pagina >= pagInicial && pag->num_pagina <= pagFinal){
			if(pag->bit_marco == NULL && pag->bit_swap == NULL){
				return true;
			}
		}
		return false;
	}
}

void unmapear(Segmento* segmento, char* idPrograma){
	bool esPrograma(Programa* prog){
		return string_equals_ignore_case(prog->id,idPrograma);
	}

	bool esSegmento(Segmento* seg){
		if(seg->es_mmap){
			return seg->base_logica == segmento->base_logica;
		}
		return false;
	}

	bool encontrarMapeo(Mapeo* mapeo){
		return string_equals_ignore_case(mapeo->path,segmento->path_mapeo);
	}

	pthread_mutex_lock(&mut_mapeos);
	Mapeo* mapeo = NULL;
	for(int i = 0; i < list_size(listaMapeos); i++){
		if(string_equals_ignore_case(((Mapeo*)list_get(listaMapeos,i))->path,segmento->path_mapeo)){
			mapeo = list_get(listaMapeos,i);
			break;
		}
	}

	if(mapeo != NULL){
		mapeo->contador--;
		//saco el segmento de la lista de segmentos del programa
		pthread_mutex_lock(&mut_listaProgramas);
		Programa* prog = list_find(listaProgramas,(void*)esPrograma);
		pthread_mutex_unlock(&mut_listaProgramas);
		list_remove_by_condition(prog->segmentos,(void*)esSegmento);

		if(mapeo->contador==0){//si no quedan mas referenciandolo se elimina
			list_remove_and_destroy_by_condition(listaMapeos,(void*)encontrarMapeo,(void*)destruirMapeo);
		}
	}

	pthread_mutex_unlock(&mut_mapeos);
}

void destruirMapeo(Mapeo* mapeo){
	free(mapeo->path);
	list_destroy_and_destroy_elements(mapeo->paginas,(void*)destruirPagina);
	free(mapeo);
}

void destruirPagina(Pagina* pag){
	if(pag->bit_marco != NULL){
		pag->bit_marco->esta_ocupado = false;
		pag->bit_marco = NULL;
	}
	if(pag->bit_swap != NULL){
		pag->bit_swap->esta_ocupado = false;
	}
	free(pag);
}

void* generarRelleno(int relleno){
	void* voidRelleno = malloc(relleno);
	char* caracter = malloc(1);
	char barraCero = '\0';
	memcpy(caracter,&barraCero,1);
	int puntero = 0;
	for(int i = 0; i < relleno; i++, puntero++){
		memcpy(voidRelleno+puntero,(void*)barraCero,1);
	}
	free(caracter);
	return voidRelleno;
}

Segmento* obtenerSegmentoPorDireccion(t_list* listaSegmentos, uint32_t direccion){
	bool buscarSegmento(Segmento* seg){
		if(direccion - seg->base_logica < seg->tamanio){
			return true;
		}
		return false;
	}

	return list_find(listaSegmentos,(void*)buscarSegmento);
}

int techo(double d){
	if(d-(int)d != 0){
		return (int)d + 1;
	}
	else{
		return (int)d;
	}
}

uint32_t calcularFramesNecesarios(uint32_t tam){
	int frames_necesarios = 0;
	float aux = ((float)(tam) / (float)TAMANIO_PAGINA);
	do{
		frames_necesarios++;
	}while(aux > frames_necesarios);
	return frames_necesarios;
}

Segmento* obtenerSegmentoParaOcupar(t_list* listaSegmentos,uint32_t tamanio){

	bool tieneHeapDisponible(InfoHeap* heap){
		if(heap->libre && heap->espacio >= tamanio){
			return true;
		}
		return false;
	}

	bool findSegmentoConEspacio(Segmento* seg){
		if(!seg->es_mmap){
			return list_any_satisfy(seg->status_metadata,(void*)tieneHeapDisponible);
		} else {
			return false;
		}
	}
	return list_find(listaSegmentos,(void*)findSegmentoConEspacio);
}

InfoHeap* obtenerHeapConEspacio(Segmento* segmento, uint32_t tamanio){
	for(int i = 0; i < list_size(segmento->status_metadata); i++){
		if(((InfoHeap*)list_get(segmento->status_metadata, i))->libre && ((InfoHeap*)list_get(segmento->status_metadata, i))->espacio >= tamanio + sizeof(HeapMetadata)){
			return list_get(segmento->status_metadata, i);
		}
	}
}

Segmento* obtenerUltimoSegmento(t_list* listaSegmentos){
	return list_get(listaSegmentos, list_size(listaSegmentos) - 1);
}

uint32_t obtenerBaseLogicaNuevoSegmento(t_list* listaSegmentos){
	if(list_is_empty(listaSegmentos)){
		return 0;
	}
	Segmento* ultimoSegmento = obtenerUltimoSegmento(listaSegmentos);
	return ultimoSegmento->base_logica + ultimoSegmento->paginas->elements_count * TAMANIO_PAGINA;
}


void sustituirHeapMetadata(InfoHeap* heapLista, Segmento* seg, HeapMetadata* new){
	int offsetMarco = heapLista->direccion_heap % TAMANIO_PAGINA;
	int paginaALaQuePertenece = heapLista->direccion_heap / TAMANIO_PAGINA;
	if(offsetMarco > TAMANIO_PAGINA - sizeof(HeapMetadata)){ //otra vez al diome xd
		int tamanioACopiar = TAMANIO_PAGINA - offsetMarco;
		Pagina* paginaHMPrimaria = list_get(seg->paginas,paginaALaQuePertenece);
		void* marcoHeapPrincipal = obtenerPunteroAMarco(paginaHMPrimaria);
		memcpy(marcoHeapPrincipal + offsetMarco, new, tamanioACopiar);

		Pagina* paginaHeapSecundaria = list_get(seg->paginas, paginaALaQuePertenece + 1);
		void* marcoHeapSecundario = obtenerPunteroAMarco(paginaHeapSecundaria);
		memcpy(marcoHeapSecundario, new + tamanioACopiar, sizeof(HeapMetadata) - tamanioACopiar);
	} else { // not the middle
		Pagina* pag = list_get(seg->paginas,paginaALaQuePertenece);
		void* punteroMarco = obtenerPunteroAMarco(pag);
		memcpy(punteroMarco + offsetMarco, new, sizeof(HeapMetadata));
	}
}

void merge(Segmento* segmentoEncontrado){

	t_list* metadatasSegmento = segmentoEncontrado->status_metadata;
	int contador = metadatasSegmento->elements_count-1;

	while(contador >= 0){
		InfoHeap* hm = list_get(metadatasSegmento,contador);
		if(hm->libre && metadatasSegmento->elements_count == 1){
			int acumuladorPaginasLiberadas = 0;
			for(int i = 0; i < segmentoEncontrado->paginas->elements_count;){
				list_remove_and_destroy_element(segmentoEncontrado->paginas, i, (void*)destruirPagina);
				acumuladorPaginasLiberadas++;
			}
			//segmentoEncontrado->paginas_liberadas+=cuantas_paginas_son_liberadas;
			segmentoEncontrado->tamanio = segmentoEncontrado->paginas->elements_count * TAMANIO_PAGINA;
			pthread_mutex_lock(&mut_espacioDisponible);
			espacioDisponible += acumuladorPaginasLiberadas * TAMANIO_PAGINA;
			pthread_mutex_unlock(&mut_espacioDisponible);
			list_remove_and_destroy_element(segmentoEncontrado->status_metadata,0,(void*)free);
		} else if(hm->libre && contador == metadatasSegmento->elements_count - 1) { // es el último, el anterior está libre
			InfoHeap* anterior = list_get(hm,contador-1);
			if(anterior->libre){
				int pagAnterior = (anterior->direccion_heap + sizeof(HeapMetadata)) / TAMANIO_PAGINA;
				int acumuladorPaginasLiberadas = 0;
				for(int i = pagAnterior + 1; i < segmentoEncontrado->paginas->elements_count;){
					list_remove_and_destroy_element(segmentoEncontrado->paginas,i,(void*)destruirPagina);
					acumuladorPaginasLiberadas++;
				}
				//segmentoEncontrado->paginas_liberadas+=cuantas_paginas_son_liberadas;
				segmentoEncontrado->tamanio = segmentoEncontrado->paginas->elements_count * TAMANIO_PAGINA;
				pthread_mutex_lock(&mut_espacioDisponible);
				espacioDisponible += acumuladorPaginasLiberadas * TAMANIO_PAGINA;
				pthread_mutex_unlock(&mut_espacioDisponible);

				int offsetAnterior = anterior->direccion_heap % TAMANIO_PAGINA;
				anterior->espacio = TAMANIO_PAGINA - offsetAnterior - sizeof(HeapMetadata);

				HeapMetadata* hmNuevo = malloc(sizeof(HeapMetadata));
				hmNuevo->libre = true;
				hmNuevo->tamanio = anterior->espacio;
				sustituirHeapMetadata(anterior,segmentoEncontrado,hmNuevo);
				if(contador>=1){
					list_remove_and_destroy_element(segmentoEncontrado->status_metadata,hm->indice,(void*)free);
				}
			}
		} else if(hm->libre && contador > 0){
			InfoHeap* anterior = list_get(metadatasSegmento, contador-1);
			if(anterior->libre){
				anterior->espacio += sizeof(HeapMetadata)+hm->espacio;
				HeapMetadata* new = malloc(sizeof(HeapMetadata));
				new->libre = true;
				new->tamanio = anterior->espacio;
				sustituirHeapMetadata(anterior,segmentoEncontrado,new);
				list_remove_and_destroy_element(segmentoEncontrado->status_metadata,hm->indice,(void*)free);
			}
		}
		contador--;
	}
}

// FUNCIONES PARA ALGORITMO CLOCK

BitSwap* buscarBitLibreSwap(){
	pthread_mutex_lock(&mut_bitmap);
	bool bitLibre(BitSwap* bit){
		return !bit->esta_ocupado;
	}
	pthread_mutex_unlock(&mut_bitmap);
	return list_find(bitmap->bits_memoria_virtual,(void*)bitLibre);
}

BitSwap* pasarASwap(BitMemoria* bit){
	char* marcoAPasar = posicionInicialMemoria + bit->pos * TAMANIO_PAGINA;
	BitSwap* bitSwap = buscarBitLibreSwap();
	char* swapAEscribir = posicionInicialSwap + bitSwap->pos * TAMANIO_PAGINA;
	for(int i=0; i<TAMANIO_PAGINA; i++){
		swapAEscribir[i] = marcoAPasar[i];
	}
	msync(posicionInicialSwap,TAMANIO_SWAP,MS_SYNC);
	fflush(stdout);
	bitSwap->esta_ocupado = true;
	return bitSwap;
}

Pagina* buscarPaginaSegunBit(BitMemoria* bit){
	Pagina* paginaVictima = NULL;
	void buscarBit(Pagina* pag){
		if(pag->presencia){
			if(pag->bit_marco->pos == bit->pos){
				paginaVictima = pag;
			}
		}
	}
	void iterarPaginas(Segmento* seg){
		list_iterate(seg->paginas,(void*)buscarBit);
	}
	void iterarSegmentos(Programa* programa){
		list_iterate(programa->segmentos,(void*)iterarPaginas);
	}
	pthread_mutex_lock(&mut_listaProgramas);
	list_iterate(listaProgramas,(void*)iterarSegmentos);
	pthread_mutex_unlock(&mut_listaProgramas);
	return paginaVictima;
}

BitMemoria* buscarBit_0_0(){
	pthread_mutex_lock(&mut_bitmap);
	int punteroInicial = punteroClock;
	BitMemoria* bitNulo = NULL;
	for(int i = 0; i < bitmap->tamanio_memoria-punteroInicial; i++){
		BitMemoria* bit = list_get(bitmap->bits_memoria,punteroClock);
		if(!bit->bit_modificado && !bit->bit_uso){
			punteroClock++;
			pthread_mutex_unlock(&mut_bitmap);
			return bit;
		}
		punteroClock++;
	}
	for(punteroClock = 0; punteroClock < punteroInicial; punteroClock++){
		BitMemoria* bit = list_get(bitmap->bits_memoria,punteroClock);
		if(!bit->bit_modificado && !bit->bit_uso){
			pthread_mutex_unlock(&mut_bitmap);
			return bit;
		}
	}
	punteroClock = punteroInicial;
	pthread_mutex_unlock(&mut_bitmap);
	return bitNulo;
}

BitMemoria* buscarBit_0_1(){
	pthread_mutex_lock(&mut_bitmap);
	int punteroInicial = punteroClock;
	BitMemoria* bitNulo = NULL;
	//busco en la primera mitad
	for(int i = 0; i < bitmap->tamanio_memoria-punteroInicial; i++){
		BitMemoria* bit = list_get(bitmap->bits_memoria,punteroClock);
		if(bit->bit_modificado && !bit->bit_uso){
			punteroClock ++;
			pthread_mutex_unlock(&mut_bitmap);
			return bit;
		}
		if(bit->bit_uso){
			bit->bit_uso = false;
		}
		punteroClock++;
	}
	for(punteroClock = 0; punteroClock < punteroInicial;punteroClock++){
		BitMemoria* bit = list_get(bitmap->bits_memoria,punteroClock);
		if(bit->bit_modificado && !bit->bit_uso){
			pthread_mutex_unlock(&mut_bitmap);
			return bit;
		}
		if(bit->bit_uso){
			bit->bit_uso = false;
		}
	}
	punteroClock = punteroInicial;
	pthread_mutex_unlock(&mut_bitmap);
	return bitNulo;
}

BitMemoria* ejecutarClockModificado(){
	BitMemoria* bitObtenido = buscarBit_0_0();
	if(bitObtenido == NULL){
		bitObtenido = buscarBit_0_1();
		if(bitObtenido == NULL){
			bitObtenido = ejecutarSegundaVueltaClock();
		}
	}

	Pagina* paginaVictima = buscarPaginaSegunBit(bitObtenido);
	if(paginaVictima != NULL){
		paginaVictima->bit_swap = pasarASwap(paginaVictima->bit_marco);
		paginaVictima->bit_marco = NULL;
		paginaVictima->presencia = false;
		bitObtenido->bit_modificado = false;//lo dejo en (1,0) listo para usar
		bitObtenido->bit_uso = true;
	}
	else{
		log_error(logger,"Error al ejecutar clock modificado: no se puede sacar una página que no se encuentra en la tabla.");
	}

	return bitObtenido;
}

BitMemoria* ejecutarSegundaVueltaClock(){
	BitMemoria* bitObtenido = buscarBit_0_0();
	if(bitObtenido == NULL){
		bitObtenido = buscarBit_0_1();
	}
	return bitObtenido;
}

void destruirPrograma(Programa* prog){
    _Bool esPrograma(Programa* programa){
        return string_equals_ignore_case(programa->id,prog->id);
    }
    list_destroy_and_destroy_elements(prog->segmentos,(void)destruirSegmento);
    list_remove_by_condition(tabla_de_programas,(void)esPrograma);
}

void destruirSegmento(Segmento* seg){
    if(!seg->es_mmap){
        list_destroy_and_destroy_elements(seg->status_metadata,(void)free);
        list_destroy_and_destroy_elements(seg->paginas,(void)destruirPagina);
    }
    free(seg);
}
