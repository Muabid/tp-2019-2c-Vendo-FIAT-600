#include <stdio.h>
#include <stdlib.h>
#include "muse.h"
#include <shared/net.h>
#include <shared/protocol.h>

int listener_socket;

int main(void) {

	init_muse_server();
	return EXIT_SUCCESS;
}
void* handler_clients(void* socket){
	int muse_sock = (int) (socket);
	while(1){
		t_message* message = recv_message(muse_sock);
		switch(message->head){

		}
		free_t_message(message);
	}
}

void init_muse_server() {
	listener_socket = init_server(PUERTO);
	log_info(logger, "Servidor levantado!!!");
	struct sockaddr muse_cli;
	socklen_t len = sizeof(muse_cli);
	do {
		int muse_sock = accept(listener_socket, &muse_cli, &len);
		if (muse_sock > 0) {
			log_info(logger, "NUEVA CONEXIÓN");
			pthread_t muse_cli_thread;
			pthread_create(&muse_cli_thread, NULL, handler_clients,
					(void*) (muse_sock));
			pthread_detach(muse_cli_thread);
		} else {
			log_error(logger, "Error aceptando conexiones: %s", strerror(errno));
		}
	} while (1);
}


void inicializarLogger(char* path){//0 es archivo, 1 es consola
	char* nombre = string_new();
	string_append(&nombre,path);
	string_append(&nombre,".log");
	logger = log_create(nombre,"muse",1,LOG_LEVEL_TRACE);
	free(nombre);
}

void cargarConfiguracion(){
	t_config* config = config_create("muse.config");
	PUERTO = config_get_int_value(config, "LISTEN_PORT");
	TAMANIO_MEMORIA = config_get_int_value(config, "MEMORY_SIZE");
	TAMANIO_PAGINA = config_get_int_value(config, "PAGE_SIZE");
	TAMANIO_SWAP = config_get_int_value(config, "SWAP_SIZE");
}

void inicializarEstructuras(){
	posicionInicialMemoria = malloc(TAMANIO_MEMORIA);
	espacioDisponible = TAMANIO_MEMORIA + TAMANIO_SWAP;
	CANT_PAGINAS_MEMORIA = TAMANIO_MEMORIA / TAMANIO_PAGINA;
	CANT_PAGINAS_SWAP = TAMANIO_SWAP / TAMANIO_PAGINA;
	listaMapeos = list_create();
	listaProgramas = list_create();
	punteroClock = 0;
	inicializarMemoriaVirtual("unaruta");
	inicializarBitmap();
	inicializarSemaforos();
}

void inicializarBitmap(){
	bitmap = malloc(sizeof(Bitmap));
	bitmap->tamanio_memoria = TAMANIO_MEMORIA;
	bitmap->bits_memoria = list_create();
	for(uint32_t i = 0; i < TAMANIO_MEMORIA; i++){
		BitMemoria* bit = malloc(sizeof(BitMemoria));
		bit->esta_ocupado = false;
		bit->pos = i;
		bit->bit_modificado = false;
		bit->bit_uso = false;
		list_add(bitmap->bits_memoria,bit);
	}

	bitmap->tamanio_memoria_virtual = TAMANIO_SWAP;
	bitmap->bits_memoria_virtual = list_create();
	for(uint32_t i = 0; i < TAMANIO_SWAP; i++){
		BitSwap* bit = malloc(sizeof(BitSwap));
		bit->esta_ocupado = false;
		bit->pos = i;
		list_add(bitmap->bits_memoria_virtual,bit);
	}

}

void inicializarMemoriaVirtual(char* rutaSwap){
	int i = strlen(rutaSwap);
	for(;i>=0;i--)
	{
		if (rutaSwap[i]=='/')
		{
			break;
		}
	}
	char* aux = string_substring_until(rutaSwap,i);
	free(rutaSwap);
	rutaSwap = string_new();
	string_append(&rutaSwap,aux);
	string_append(&rutaSwap,"/AreaSwap");
	log_info(logger,"Ruta area Swap = %s",rutaSwap);
	free(aux);

	fileDescriptor = open(rutaSwap,O_RDWR|O_CREAT,0777);
	if(fileDescriptor<0){
		log_info(logger,"F en el chat por el archivo de swap que no se pudo abrir");
	}
	ftruncate(fileDescriptor,0);
	ftruncate(fileDescriptor,TAMANIO_SWAP);
	posicionInicialSwap = mmap(NULL, TAMANIO_SWAP, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fileDescriptor, 0);
	if(posicionInicialSwap == MAP_FAILED || posicionInicialSwap == NULL){
		perror("error: ");
	}

}

void inicializarSemaforos(){
	pthread_mutex_init(&mut_espacioDisponible,NULL);
	pthread_mutex_init(&mut_listaProgramas,NULL);
	pthread_mutex_init(&mut_bitmap,NULL);
}

int muse_alloc(char* id, uint32_t tamanio){
	int theReturn;
	pthread_mutex_lock(&mut_espacioDisponible);
	if(espacioDisponible >= tamanio + sizeof(HeapMetadata)){
		pthread_mutex_unlock(&mut_espacioDisponible);
		t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
		if(listaSegmentos==NULL){
			log_info(logger,"no hay segmentos, F",id);
			return -1;
		}

		if(list_is_empty(listaSegmentos)){
			uint32_t paginasNecesarias = calcularFramesNecesarios(tamanio + sizeof(HeapMetadata) * 2);
			int espacioLibreUltimaPagina = paginasNecesarias * TAMANIO_PAGINA - tamanio - sizeof(HeapMetadata) * 2;
			pthread_mutex_lock(&mut_espacioDisponible);
			if(espacioDisponible >= paginasNecesarias * TAMANIO_PAGINA){
				espacioDisponible -= paginasNecesarias * TAMANIO_PAGINA;
				pthread_mutex_unlock(&mut_espacioDisponible);
				//creo segmento
				Segmento* nuevoSegmento = malloc(sizeof(Segmento));
				nuevoSegmento->compartido = false;
				nuevoSegmento->es_mmap = false;
				nuevoSegmento->base_logica = 0;
				nuevoSegmento->tamanio = paginasNecesarias * TAMANIO_PAGINA;
				nuevoSegmento->status_metadata = list_create();

				//inicializo su status de hm, hay q agregar 2 porque el segmento es nuevo !!
				InfoHeap* primerHeap = malloc(sizeof(InfoHeap));
				primerHeap->direccion_heap = 0;
				primerHeap->espacio = tamanio;
				primerHeap->libre = false;
				primerHeap->indice = 0;
				InfoHeap* segundoheap = malloc(sizeof(InfoHeap));
				segundoheap->direccion_heap = tamanio + sizeof(HeapMetadata);
				segundoheap->libre = true;
				segundoheap->espacio = espacioLibreUltimaPagina;
				segundoheap->indice = 1;

				// los agrego al segmento recien creado
				list_add(nuevoSegmento->status_metadata, primerHeap);
				list_add(nuevoSegmento->status_metadata, segundoheap);

				bool hayQueEntrarALaAnteultimaPag = false; // flag q va a servir

				t_list* paginasCreadas = list_create();

				int heapUltimaPagina;
				int heapAnteultimaPagina;
				int offset;
				HeapMetadata* heapFinal = malloc(sizeof(HeapMetadata));
				heapFinal->libre = true;
				heapFinal->tamanio = espacioLibreUltimaPagina;

				if(espacioLibreUltimaPagina > TAMANIO_PAGINA - sizeof(HeapMetadata)) { // el HM está cortado en dos frames distintos
					heapUltimaPagina = TAMANIO_PAGINA - espacioLibreUltimaPagina;
					heapAnteultimaPagina = sizeof(HeapMetadata) - heapUltimaPagina;
					offset = TAMANIO_PAGINA - heapAnteultimaPagina;
					hayQueEntrarALaAnteultimaPag = true;
				} else { // no se parte el HM
					offset = TAMANIO_PAGINA - espacioLibreUltimaPagina - sizeof(HeapMetadata);
				}

				for(int i = 0; i < paginasNecesarias; i++){
					Pagina* pag = malloc(sizeof(Pagina));
					pag->num_pagina = i;
					pag->presencia = true;
					if(i <= TAMANIO_MEMORIA / TAMANIO_PAGINA){
						pag->bit_marco = asignarMarcoNuevo();
						pag->bit_marco->bit_uso = true;
						pag->bit_marco->bit_modificado = true;
						pag->bit_swap = NULL;
					}
					else{
						pag->bit_marco = NULL;
						pag->bit_swap = buscarBitLibreSwap();
						pag->presencia = false;
					}
					if(i == 0){
						HeapMetadata* heap = malloc(sizeof(HeapMetadata));
						heap->libre = false;
						heap->tamanio = tamanio;
						void* punteroMarco = obtenerPunteroAMarco(pag);
						memcpy(punteroMarco, heap, sizeof(HeapMetadata));
						free(heap);
					}
					if(i == paginasNecesarias - 2){
						if(hayQueEntrarALaAnteultimaPag){
							void* punteroMarco = obtenerPunteroAMarco(pag);
							memcpy(punteroMarco + offset, heapFinal, heapAnteultimaPagina);
						}
					}
					if(i == paginasNecesarias-1){
						void* punteroMarco = obtenerPunteroAMarco(pag);
						if(hayQueEntrarALaAnteultimaPag){
							memcpy(punteroMarco, heapFinal + heapAnteultimaPagina, heapUltimaPagina);
						}
						else{
							memcpy(punteroMarco + offset, heapFinal, sizeof(HeapMetadata));
						}
					}
					list_add(paginasCreadas,pag);
				}
				nuevoSegmento->num_segmento = list_size(listaSegmentos);
				nuevoSegmento->paginas = paginasCreadas;
				list_add(listaSegmentos,nuevoSegmento);
				free(heapFinal);

				return sizeof(HeapMetadata);

			} else {
				puts("El tamanio solicitado no entra");
				log_info(logger,"big F no hay lugar");
				pthread_mutex_unlock(&mut_espacioDisponible);
				return -1;
			}
		} else { // ya hay algún segmento, entonces busco en la lista de heaps de cada segmento a ver si hay espacio para alocar
			Segmento* segmentoEncontrado = obtenerSegmentoParaOcupar(listaSegmentos,tamanio + sizeof(HeapMetadata));
			if(segmentoEncontrado != NULL){
				// me pongo a buscar el espacio disponible
				InfoHeap* heapConEspacio = obtenerHeapConEspacio(segmentoEncontrado,tamanio);

				HeapMetadata* heapInicial = malloc(sizeof(HeapMetadata));
				heapInicial->libre = false;
				heapInicial->tamanio = tamanio;
				int numPaginaInicial = heapConEspacio->direccion_heap / TAMANIO_PAGINA;
				int offsetInicial = heapConEspacio->direccion_heap % TAMANIO_PAGINA;

				if(offsetInicial > TAMANIO_PAGINA - sizeof(HeapMetadata)){ // queda en el diome
					int loQueCopio = TAMANIO_PAGINA - offsetInicial;

					Pagina* paginaPrincipal = list_get(segmentoEncontrado->paginas, numPaginaInicial);
					void* marcoPrincipal = obtenerPunteroAMarco(paginaPrincipal);
					memcpy(marcoPrincipal + offsetInicial, heapInicial, loQueCopio);

					Pagina* paginaSecundaria = list_get(segmentoEncontrado->paginas, numPaginaInicial + 1);
					void* marcoSecundario = obtenerPunteroAMarco(paginaSecundaria);
					memcpy(marcoSecundario, heapInicial + loQueCopio, sizeof(HeapMetadata) - loQueCopio);

					theReturn = segmentoEncontrado->base_logica + paginaSecundaria->num_pagina * TAMANIO_PAGINA + sizeof(HeapMetadata) - loQueCopio;

				} else { // no queda en el diome
					Pagina* paginaPrincipal = list_get(segmentoEncontrado->paginas, numPaginaInicial);
					void* marcoPrincipal = obtenerPunteroAMarco(paginaPrincipal);
					memcpy(marcoPrincipal + offsetInicial, heapInicial, sizeof(HeapMetadata));

					theReturn = segmentoEncontrado->base_logica + paginaPrincipal->num_pagina * TAMANIO_PAGINA + offsetInicial + sizeof(HeapMetadata);
				}

				HeapMetadata* heapFinal = malloc(sizeof(HeapMetadata));
				heapFinal->libre = true;
				heapFinal->tamanio = heapConEspacio->espacio - tamanio - sizeof(HeapMetadata);
				int numPaginaFinal = (heapConEspacio->direccion_heap + tamanio + sizeof(HeapMetadata)) / TAMANIO_PAGINA;
				int offsetFinal = (heapConEspacio->direccion_heap + tamanio + sizeof(HeapMetadata)) % TAMANIO_PAGINA;

				if(offsetFinal > TAMANIO_PAGINA - sizeof(HeapMetadata)){ // QEDA en el diome again
					int loQueCopio = TAMANIO_PAGINA - offsetFinal;

					Pagina* paginaFinalPrincipal = list_get(segmentoEncontrado->paginas,numPaginaFinal);
					void* marcoFinalPrincipal = obtenerPunteroAMarco(paginaFinalPrincipal);
					memcpy(marcoFinalPrincipal + offsetFinal, heapFinal, loQueCopio);

					Pagina* paginaFinalSecundaria = list_get(segmentoEncontrado->paginas,numPaginaFinal + 1);
					void* marcoFinalSecundario = obtenerPunteroAMarco(paginaFinalSecundaria);
					memcpy(marcoFinalSecundario, heapFinal + loQueCopio, sizeof(HeapMetadata) - loQueCopio);
				} else {// no queda en el diome graciadio
					Pagina* paginaFinal = list_get(segmentoEncontrado->paginas,numPaginaFinal);
					void* marcoFinal = obtenerPunteroAMarco(paginaFinal);
					memcpy(marcoFinal + offsetInicial, heapFinal, sizeof(HeapMetadata));
				}

				heapConEspacio->espacio = tamanio;
				heapConEspacio->libre = false;

				InfoHeap* siguienteHeap = malloc(sizeof(heapConEspacio));
				siguienteHeap->direccion_heap = numPaginaFinal * TAMANIO_PAGINA + offsetFinal;
				siguienteHeap->espacio = heapFinal->tamanio;
				siguienteHeap->libre = true;
				siguienteHeap->indice = heapConEspacio->indice + 1;
				// lo agrego al final by: frongi wtf
				list_add_in_index(segmentoEncontrado->status_metadata, heapConEspacio->indice+1, siguienteHeap);

				free(heapInicial);
				free(heapFinal);

			} else { // no encontró ningun segmento, hay que ver si se puede agrandar o crear otro //acá tambien podria ir lo de liberadas
				Segmento* ultimoSegmento = obtenerUltimoSegmento(listaSegmentos);
				if(ultimoSegmento->es_mmap){
					// acá no puedo agrandar asique creo otro segmento xd
					// ¿delegar funcion crear segmento? yo diria que no kent
					uint32_t paginasNecesarias = calcularFramesNecesarios(tamanio + sizeof(HeapMetadata)*2);
					int libreUltimaPag = paginasNecesarias * TAMANIO_PAGINA - tamanio - sizeof(HeapMetadata) * 2;
					uint32_t tamTotal = paginasNecesarias * TAMANIO_PAGINA;
					pthread_mutex_lock(&mut_espacioDisponible);
					if(espacioDisponible >= tamTotal){
						espacioDisponible -= tamTotal;
						pthread_mutex_unlock(&mut_espacioDisponible);

						Segmento* nuevoSegmento = malloc(sizeof(Segmento));
						nuevoSegmento->compartido = false;
						nuevoSegmento->es_mmap = false;
						nuevoSegmento->tamanio = paginasNecesarias * TAMANIO_PAGINA;
						nuevoSegmento->status_metadata = list_create();

						//inicializo su status de hm, hay q agregar 2 porque el segmento es nuevo !!
						InfoHeap* primerHeap = malloc(sizeof(InfoHeap));
						primerHeap->direccion_heap = 0;
						primerHeap->espacio = tamanio;
						primerHeap->libre = false;
						primerHeap->indice = 0;
						InfoHeap* segundoheap = malloc(sizeof(InfoHeap));
						segundoheap->direccion_heap = tamanio + sizeof(HeapMetadata);
						segundoheap->libre = true;
						segundoheap->espacio = libreUltimaPag;
						segundoheap->indice = 1;

						// los agrego al segmento recien creado
						list_add(nuevoSegmento->status_metadata, primerHeap);
						list_add(nuevoSegmento->status_metadata, segundoheap);

						bool hayQueEntrarALaAnteultimaPag = false; // flag q va a servir

						t_list* paginasCreadas = list_create();

						int heapUltimaPagina;
						int heapAnteultimaPagina;
						int offset;
						HeapMetadata* heapFinal = malloc(sizeof(HeapMetadata));
						heapFinal->libre = true;
						heapFinal->tamanio = libreUltimaPag;

						if(libreUltimaPag > TAMANIO_PAGINA - sizeof(HeapMetadata)) { // el HM está cortado en dos frames distintos
							heapUltimaPagina = TAMANIO_PAGINA - libreUltimaPag;
							heapAnteultimaPagina = sizeof(HeapMetadata) - heapUltimaPagina;
							offset = TAMANIO_PAGINA - heapAnteultimaPagina;
							hayQueEntrarALaAnteultimaPag = true;
						} else { // no se parte el HM
							offset = TAMANIO_PAGINA - libreUltimaPag - sizeof(HeapMetadata);
						}

						for(int i = 0; i < paginasNecesarias; i++){
							Pagina* pag = malloc(sizeof(Pagina));
							pag->num_pagina = i;
							pag->presencia = true;
							if(i <= TAMANIO_MEMORIA / TAMANIO_PAGINA){
								pag->bit_marco = asignarMarcoNuevo();
								pag->bit_marco->bit_uso = true;
								pag->bit_marco->bit_modificado = true;
								pag->bit_swap = NULL;
							}
							else{
								pag->bit_marco = NULL;
								pag->bit_swap = buscarBitLibreSwap();
								pag->presencia = false;
							}
							if(i == 0){
								HeapMetadata* heap = malloc(sizeof(HeapMetadata));
								heap->libre = false;
								heap->tamanio = tamanio;
								void* punteroMarco = obtenerPunteroAMarco(pag);
								memcpy(punteroMarco, heap, sizeof(HeapMetadata));
								free(heap);
							}
							if(i == paginasNecesarias - 2){
								if(hayQueEntrarALaAnteultimaPag){
									void* punteroMarco = obtenerPunteroAMarco(pag);
									memcpy(punteroMarco + offset, heapFinal, heapAnteultimaPagina);
								}
							}
							if(i == paginasNecesarias-1){
								void* punteroMarco = obtenerPunteroAMarco(pag);
								if(hayQueEntrarALaAnteultimaPag){
									memcpy(punteroMarco, heapFinal + heapAnteultimaPagina, heapUltimaPagina);
								}
								else{
									memcpy(punteroMarco + offset, heapFinal, sizeof(HeapMetadata));
								}
							}
							list_add(paginasCreadas,pag);
						}
						nuevoSegmento->num_segmento = list_size(listaSegmentos);
						nuevoSegmento->paginas = paginasCreadas;
						nuevoSegmento->base_logica = obtenerBaseLogicaNuevoSegmento(listaSegmentos);
						list_add(listaSegmentos,nuevoSegmento);
						free(heapFinal);

						theReturn = nuevoSegmento->base_logica + sizeof(HeapMetadata);
					}
				} else { // acá puedo agrandar
					InfoHeap* ultimoInfoHeap = obtenerUltimoSegmento(ultimoSegmento->status_metadata);
					ultimoInfoHeap->libre = false;
					Pagina* ultimaPagina = obtenerUltimoSegmento(ultimoSegmento->paginas);
					HeapMetadata* nuevoUltHeap = malloc(sizeof(HeapMetadata));
					nuevoUltHeap->libre = true;
					HeapMetadata* ultimoHeap = malloc(sizeof(HeapMetadata));
					ultimoHeap->libre = false;
					ultimoHeap->tamanio = tamanio;
					uint32_t loQueNecesito = tamanio - ultimoInfoHeap->espacio + sizeof(HeapMetadata);
					uint32_t paginasNecesarias = calcularFramesNecesarios(loQueNecesito);
					uint32_t tamanioEntero = paginasNecesarias * TAMANIO_PAGINA;
					pthread_mutex_lock(&mut_espacioDisponible);
					if(espacioDisponible >= tamanioEntero){
						espacioDisponible -= tamanioEntero;
						pthread_mutex_unlock(&mut_espacioDisponible);
						ultimoSegmento->tamanio += tamanioEntero;
						int libreUltimaPagina = tamanioEntero - tamanio - sizeof(HeapMetadata) + ultimoInfoHeap->espacio;
						// actualizar anteultimo heap grascias
						ultimoInfoHeap->espacio = tamanio;
						nuevoUltHeap->tamanio = libreUltimaPagina;

						//agrego las pginas y los marcos y todo eso man hay que delegar esta chota? NO COSCU

						for(int i = 0; i < paginasNecesarias; i++){
							Pagina* newPagerino = malloc(sizeof(Pagina));
							newPagerino->num_pagina = ultimaPagina->num_pagina + 1 + i; // meme del negro ??
							newPagerino->presencia = true;

							if(i <= TAMANIO_MEMORIA / TAMANIO_PAGINA){
								newPagerino->bit_marco = asignarMarcoNuevo();
								newPagerino->bit_marco->bit_uso = true;
								newPagerino->bit_marco->bit_modificado = true;
								newPagerino->bit_swap = NULL;
							} else {
								newPagerino->bit_marco = NULL;
								newPagerino->bit_swap = buscarBitLibreSwap();
								newPagerino->presencia = false;
							}
							list_add(ultimoSegmento->paginas,newPagerino);

						}

						InfoHeap* nuevoInfoHeaperino = malloc(sizeof(InfoHeap));
						nuevoInfoHeaperino->direccion_heap = ultimoInfoHeap->direccion_heap + tamanio + sizeof(HeapMetadata);
						nuevoInfoHeaperino->espacio = libreUltimaPagina;
						nuevoInfoHeaperino->indice = ultimoInfoHeap->indice + 1; // claro pq lo corrí
						nuevoInfoHeaperino->libre = true;
						list_add_in_index(ultimoSegmento->status_metadata, nuevoInfoHeaperino->indice, nuevoInfoHeaperino);

						// esto vendria a ser el HM nuevo
						int offsetNuevoXD = nuevoInfoHeaperino->direccion_heap % TAMANIO_PAGINA;
						int numPaginaFinal = nuevoInfoHeaperino->direccion_heap / TAMANIO_PAGINA;
						if(offsetNuevoXD > TAMANIO_PAGINA - sizeof(HeapMetadata)){ //oh shit here we go again al diome
							int loQueCopio = TAMANIO_PAGINA - offsetNuevoXD;
							Pagina* paginaFinalPrincipal = list_get(ultimoSegmento->paginas, numPaginaFinal);
							void* marcoFinalPrincipal = obtenerPunteroAMarco(paginaFinalPrincipal);
							memcpy(marcoFinalPrincipal + offsetNuevoXD, nuevoUltHeap, loQueCopio); // el nuevo heap viene de arriba, no tocar

							Pagina* paginaFinalSecundaria = list_get(ultimoSegmento->paginas, numPaginaFinal + 1);
							void* marcoFinalSecundario = obtenerPunteroAMarco(paginaFinalSecundaria);
							memcpy(marcoFinalSecundario, nuevoUltHeap + loQueCopio, sizeof(HeapMetadata) - loQueCopio);
						} else {// like y me mato
							Pagina* paginaFinal = list_get(ultimoSegmento->paginas, numPaginaFinal);
							void* punteroMarcoUltimaPagina = obtenerPunteroAMarco(paginaFinal);
							memcpy(punteroMarcoUltimaPagina + offsetNuevoXD, nuevoUltHeap, sizeof(HeapMetadata));
						}

						// y esto vendria a ser el HM viejardo

						int offsetViejoXD = nuevoInfoHeaperino->direccion_heap % TAMANIO_PAGINA;
						int numPaginaViejaFinal = techo((double)nuevoInfoHeaperino->direccion_heap / TAMANIO_PAGINA);
						if(offsetViejoXD > TAMANIO_PAGINA - sizeof(HeapMetadata)){ // ESCRIBÍ ESTO MIENTRAS LLORABA
							int loQueCopio2 = TAMANIO_PAGINA - offsetViejoXD;
							Pagina* paginaFinalViejaPrincipal = list_get(ultimoSegmento->paginas,numPaginaViejaFinal);
							void* marcoPrincipalViejoFinal = obtenerPunteroAMarco(paginaFinalViejaPrincipal);
							memcpy(marcoPrincipalViejoFinal + offsetViejoXD, ultimoHeap, loQueCopio2);

							Pagina* paginaFinalViejaSecundaria = list_get(ultimoSegmento->paginas,numPaginaViejaFinal + 1);
							void* marcoSecundarioViejoFinal = obtenerPunteroAMarco(paginaFinalViejaSecundaria);
							memcpy(marcoSecundarioViejoFinal, ultimoHeap + loQueCopio2, sizeof(HeapMetadata)-loQueCopio2);
						} else {
							//no queda en el medio
							Pagina* paginaFinalVieja = list_get(ultimoSegmento->paginas,numPaginaViejaFinal);
							void* punteroViejoUltimaPagina = obtenerPunteroAMarco(paginaFinalVieja);
							memcpy(punteroViejoUltimaPagina + offsetViejoXD, ultimoHeap, sizeof(HeapMetadata));
						}

						theReturn = ultimoSegmento->base_logica + sizeof(HeapMetadata) + nuevoInfoHeaperino->direccion_heap;
						free(ultimoHeap);
						free(nuevoUltHeap);
					} else {
						log_info(logger,"No hay espacio F");
						pthread_mutex_unlock(&mut_espacioDisponible);
						return -1;
					}
				}
			}
		}

		return theReturn;

	} else {
		pthread_mutex_unlock(&mut_espacioDisponible);
		log_info(logger,"F NO HAY ESPACIO");
		return -1;
	}
}

int muse_free(char* id, uint32_t dir){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL){
		log_info(logger,"F no se encontro la tabla de segmentos de %s",id);
		return -1;
	}
	Segmento* segmentoObtenido = obtenerSegmentoPorDireccion(listaSegmentos,dir);
	if(segmentoObtenido == NULL){
		log_info(logger,"F no se encontro segmento de %s",id);
		return -1;
	}

	InfoHeap* heapEncontrado = NULL;
	int direccionSegmentoObtenido = dir - segmentoObtenido->base_logica;
	for(int i = 0; i < list_size(segmentoObtenido->status_metadata); i++) {
		InfoHeap* auxiliar = list_get(segmentoObtenido->status_metadata,i);
		if(auxiliar->direccion_heap + sizeof(HeapMetadata) == direccionSegmentoObtenido) {
			heapEncontrado = auxiliar;
			heapEncontrado->libre = true;
			break;
		}
	}

	HeapMetadata* newHM = malloc(sizeof(HeapMetadata));
	newHM->libre = true;
	newHM->tamanio = heapEncontrado->espacio;
	sustituirHeapMetadata(heapEncontrado,segmentoObtenido,newHM);

	merge(segmentoObtenido);

	for(int i = 0; i < list_size(segmentoObtenido->status_metadata); i++){
		InfoHeap* heap = list_get(segmentoObtenido->status_metadata,i);
		heap->indice=i;
	}

	log_info(logger,"Direccion %i liberada correctamente (segmento %s)",dir,id);
	return 0;

}

int muse_map(char* id, char* path, uint32_t length, uint32_t flag){
	int tamanioAMapear = techo(length) * TAMANIO_PAGINA;
	pthread_mutex_lock(&mut_espacioDisponible);
	if(espacioDisponible >= tamanioAMapear){
		espacioDisponible -= tamanioAMapear;
		pthread_mutex_unlock(&mut_espacioDisponible);
		t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
		if(listaSegmentos == NULL){
			log_info(logger,"F por el programa %s que no existe",id);
			return -1;
		}
		Segmento* segmentoNuevo = malloc(sizeof(Segmento));
		segmentoNuevo->num_segmento = listaSegmentos->elements_count;
		segmentoNuevo->es_mmap = true;
		segmentoNuevo->tamanio_mapeo = length;
		segmentoNuevo->base_logica = asignarMarcoNuevo(listaSegmentos);
		//segmentoNuevo->info_heaps = NULL;
		//segmentoNuevo->paginas_liberadas=0;
		segmentoNuevo->path_mapeo = string_duplicate(path);
		segmentoNuevo->tamanio = tamanioAMapear;
		if(flag != MAP_PRIVATE){
			segmentoNuevo->compartido = true;
			t_list* paginas = obtenerMapeoExistente(path,length);
			if(paginas != NULL){
				segmentoNuevo->paginas = paginas;
				list_add(listaSegmentos,segmentoNuevo);
				return segmentoNuevo->base_logica;
			}
		}
		else{
			segmentoNuevo->compartido = false;
		}
		//tengo que crear el el mapeo de cero y llenar lista de mapeo
		t_list* paginas = list_create();
		int cantidad_de_paginas = techo(length);
		for(int i = 0; i < cantidad_de_paginas; i++){
			Pagina* pag = malloc(sizeof(Pagina));
			pag->num_pagina = i;
			pag->bit_marco = NULL;
			pag->presencia = false;
			pag->bit_swap = NULL;
			list_add(paginas,pag);
		}
		segmentoNuevo->paginas = paginas;
		list_add(listaSegmentos,segmentoNuevo);

		Mapeo* mapeo = malloc(sizeof(Mapeo));
		mapeo->contador = 1;
		mapeo->paginas = paginas;
		mapeo->path = string_duplicate(path);
		mapeo->tamanio = length;
		//mapeo->tamanio_de_pags = segmentoNuevo->tamanio;
		pthread_mutex_lock(&mut_mapeos);
		list_add(listaMapeos,mapeo);
		pthread_mutex_unlock(&mut_mapeos);
		return segmentoNuevo->base_logica;
	}
	else{
		pthread_mutex_unlock(&mut_espacioDisponible);
		//no hay lugar
		return -1;
	}
}

int muse_sync(char* id, uint32_t addr, size_t len){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL || list_is_empty(listaSegmentos)){
		log_info(logger,"F por el programa %s que no existe o no tiene segmentos",id);
		return -1;
	}

	Segmento* segmentoEncontrado = segmentoAlQuePertenece(listaSegmentos,addr);
	if(segmentoEncontrado == NULL){
		log_info(logger,"F por el programa %s que no encontro el segmento",id);
		return -1;
	}
	int dirAlSegmento = addr - segmentoEncontrado->base_logica;
		if(segmentoEncontrado->es_mmap){
			int primerPag = dirAlSegmento / TAMANIO_PAGINA;
			int ultimaPag = techo((dirAlSegmento + len) / TAMANIO_PAGINA) - 1;
			int cantidadPags = ultimaPag - primerPag + 1;
			int tamSync = cantidadPags * TAMANIO_PAGINA;

			void* auxiliar = malloc(tamSync);
			int puntero = 0;

			paginasMapEnMemoria(dirAlSegmento,len,segmentoEncontrado);

			int i = primerPag;
			while(i <= ultimaPag){
				Pagina* pag = list_get(segmentoEncontrado->paginas,i);
				void* punteroMarco = obtenerPunteroAMarco(pag);
				memcpy(auxiliar+puntero,punteroMarco,TAMANIO_PAGINA);
				i++;
				puntero += TAMANIO_PAGINA;
				tamSync -= TAMANIO_PAGINA;
			}
			int posInicial = primerPag * TAMANIO_PAGINA;
			FILE* file = fopen(segmentoEncontrado->path_mapeo,"r+");
			if(file != NULL){
				if(fseek(file,posInicial,SEEK_SET == 0)){
					fwrite(auxiliar,cantidadPags*TAMANIO_PAGINA,1,file);
					free(auxiliar);
					fclose(file);
					return 0;
				}
			}
			free(auxiliar);
		}
		log_info(logger,"F porque el segmento no es mmap",id); // si llega aca me suicido
		return -1;
}

int muse_unmap(char* id, uint32_t dir){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL){
		log_info(logger,"F por el programa %s, no se encontró su lista de segmentos", id);
		return -1;
	}
	Segmento* segmentoEncontrado = segmentoAlQuePertenece(listaSegmentos,dir);
	if(segmentoEncontrado != NULL){
		if(segmentoEncontrado->es_mmap){
			unmapear(segmentoEncontrado,id);
			return 0;
		}
	}
	perror("FFFFFFFF");
	return -1;
}

void* muse_get(char* id, uint32_t src, size_t n){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL || list_is_empty(listaSegmentos)){
		log_info(logger,"F por el programa %s que no existe o no tiene segmentos",id);
		return -1;
	}

	Segmento* segmentoEncontrado = obtenerSegmentoPorDireccion(listaSegmentos,src);
	if(segmentoEncontrado == NULL){
		//no existe el segmento buscado o se paso del segmento
		log_info(logger,"F - no se encontro segmento de %s",id);
		return -1;
	}
	int direccionAlSegmento = src - segmentoEncontrado->base_logica;
	void* resultado_muse_get = NULL;

	int pagInicial = direccionAlSegmento / TAMANIO_PAGINA;
	int pagFinal = techo((direccionAlSegmento + n) / TAMANIO_PAGINA) - 1;
	int offset = direccionAlSegmento % TAMANIO_PAGINA;

	if(segmentoEncontrado != NULL && segmentoEncontrado->tamanio - direccionAlSegmento >= n &&
	pagInicial * TAMANIO_PAGINA + offset + n <= segmentoEncontrado->tamanio){

		int cantidadPaginas = pagFinal - pagInicial + 1;
		int tamanioTotal = cantidadPaginas * TAMANIO_PAGINA;
		resultado_muse_get = malloc(n);
		void* bloquesote = malloc(tamanioTotal);
		int puntero = 0;

		if(segmentoEncontrado->es_mmap){
			//puede haber pags q nunca se levantaron (no estan en memoria)
			paginasMapEnMemoria(direccionAlSegmento,n,segmentoEncontrado);
		}
		int i = 0;
		while(i < cantidadPaginas){
			Pagina* pag = list_get(segmentoEncontrado->paginas,pagInicial + i);
			void* punteroMarco = obtenerPunteroAMarco(pag);
			pag->bit_marco->bit_uso = true;
			pag->presencia = true;
			memcpy(bloquesote + puntero, punteroMarco, TAMANIO_PAGINA);

			puntero += TAMANIO_PAGINA;
			i++;
		}

		memcpy(resultado_muse_get, bloquesote + offset, n);
		free(bloquesote);

	}
	log_info(logger,"La cosa que pidieron pasar es: %s",(char*)resultado_muse_get);
	return resultado_muse_get;

}

void* muse_cpy(char* id, uint32_t dst, void* src, size_t n){
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL || list_is_empty(listaSegmentos)){
		log_info(logger,"F por el programa %s que no existe o no tiene segmentos",id);
		return -1;
	}

	Segmento* segmentoEncontrado = obtenerSegmentoPorDireccion(listaSegmentos,src);
	if(segmentoEncontrado == NULL){
		log_info(logger,"no se encontro segmento de %s",id);
		return -1;
	}

	InfoHeap* metadataDestino = NULL;

	int direccionSegmento = dst - segmentoEncontrado->base_logica;
	if(!segmentoEncontrado->es_mmap){
		if(list_size(segmentoEncontrado->status_metadata) > 0){
			for(int i = 0; i < list_size(segmentoEncontrado->status_metadata); i++) {
				InfoHeap* metadataAux = list_get(segmentoEncontrado->status_metadata,i);
				if(metadataAux->direccion_heap + sizeof(HeapMetadata) <= direccionSegmento &&
					metadataAux->direccion_heap + sizeof(HeapMetadata) + metadataAux->espacio > direccionSegmento && !metadataAux->libre)
				{
					metadataDestino = metadataAux;
					break;
				}
			}
		}
		if(metadataDestino == NULL){
			log_info(logger,"F por la direccion %i en el programa %s",dst,id); //segfault?
			return -1;
		}

		int numPagina = direccionSegmento / TAMANIO_PAGINA;
		Pagina* pagina = list_get(segmentoEncontrado->paginas,numPagina);
		void* punteroMarco = obtenerPunteroAMarco(pagina);
		pagina->bit_marco->bit_uso = true;
		pagina->bit_marco->bit_modificado = true;
		int sobrante = direccionSegmento % TAMANIO_PAGINA;
		int loQuePuedo = TAMANIO_PAGINA - sobrante;
		if(loQuePuedo <= n){
			memcpy(punteroMarco + sobrante, src, loQuePuedo);
		} else {
			memcpy(punteroMarco + sobrante, src, n);
			log_info(logger,"Los datos se pegaron en segmento %d, direccion %d",segmentoEncontrado->num_segmento,dst);
			return 0;
		}

		// esto es en caso de que entre al if de arriba

		numPagina++; //siguiente pagina
		do{
			int loQueFalta = n - loQuePuedo;

			if(loQueFalta >= TAMANIO_PAGINA){
				Pagina* pag = list_get(segmentoEncontrado->paginas,numPagina);
				void* punteroMarco = obtenerPunteroAMarco(pag);
				pag->bit_marco->bit_uso = true;
				pag->bit_marco->bit_modificado = true;
				memcpy(punteroMarco, src + loQuePuedo, TAMANIO_PAGINA);
				loQuePuedo += TAMANIO_PAGINA;
				numPagina++;
			}
			else if(loQueFalta > 0){
				Pagina* pag = list_get(segmentoEncontrado->paginas,numPagina);
				void* punteroMarco = obtenerPunteroAMarco(pag);
				pag->bit_marco->bit_uso = true;
				pag->bit_marco->bit_modificado = true;
				memcpy(punteroMarco,src + loQuePuedo, loQueFalta);
				break;
			}
			else{
				log_info(logger,"Si entra acá me pego un corchazo en las re pelotas");
				log_info(logger,"Podría ser peor igual");
				log_info(logger,"idontwannabeyouanymore");
				break;
			}
		}while (1);

		log_info(logger,"Los datos se pegaron en segmento %d, direccion %d",segmentoEncontrado->num_segmento,dst);
		return 0;
	} else {
		paginasMapEnMemoria(direccionSegmento,n,segmentoEncontrado);
		int pagInicial = direccionSegmento / TAMANIO_PAGINA;
		int offset = direccionSegmento % TAMANIO_PAGINA;
		int pagFinal = (direccionSegmento + n) / TAMANIO_PAGINA;
		if(pagInicial * TAMANIO_PAGINA + offset + n	> segmentoEncontrado->tamanio){
			return -1;
		}
		int puntero = 0;
		for(int i = pagInicial ; i <= pagFinal; i++){
			Pagina* pag = list_get(segmentoEncontrado->paginas, i);
			void* punteroMarco = obtenerPunteroAMarco(pag);
			pag->bit_marco->bit_modificado = true;
			pag->bit_marco->bit_uso = true;
			if(pag->num_pagina == pagInicial){//si es la primera pego desde el off
				if(pag->num_pagina == pagFinal){//es la primera y ulitma
					memcpy(punteroMarco + offset, src, n);
				}
				else{
					memcpy(punteroMarco + offset, src, TAMANIO_PAGINA - offset);
					puntero += TAMANIO_PAGINA - offset;
				}
			}
			else if(pag->num_pagina == pagFinal){
				memcpy(punteroMarco, src + puntero, n - puntero);
			}
			else{
				memcpy(punteroMarco,src + puntero, TAMANIO_PAGINA);
				puntero += TAMANIO_PAGINA;
			}
		}

		return 0;
	}
}
