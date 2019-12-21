#include "muse.h"

int main(int argc, char **argv){
	rutaSwapping = string_duplicate(argv[0]);
	cargarConfiguracion();
	inicializarEstructuras(rutaSwapping);
//	inicializarLogger(string_duplicate(argv[1]));
	inicializarLogger("./Debug");
	init_muse_server();
//	recursiva(10);
	return EXIT_SUCCESS;
}

void cargarConfiguracion(){
	t_config* config = config_create("./muse.config");
	PUERTO = config_get_int_value(config, "LISTEN_PORT");
	TAMANIO_MEMORIA = config_get_int_value(config, "MEMORY_SIZE");
	TAMANIO_PAGINA = config_get_int_value(config, "PAGE_SIZE");
	TAMANIO_SWAP = config_get_int_value(config, "SWAP_SIZE");
}

void* handler_clients(void* socket){
	int muse_sock = (int) (socket);
	char* id_cliente;
	bool executing = true;
	while(executing){
		t_message* message = recv_message(muse_sock);
		switch(message->head){
			case MUSE_INIT:{
				uint32_t pid = *((uint32_t*)message->content);
				id_cliente = string_itoa(pid);
				log_info(logger,"Cliente, id: %s",id_cliente);
				Programa* programa = malloc(sizeof(Programa));
				programa->segmentos = list_create();
				programa->id = id_cliente;
				pthread_mutex_lock(&mut_listaProgramas);
				list_add(listaProgramas,programa);
				pthread_mutex_unlock(&mut_listaProgramas);
				send_message(muse_sock,OK,&pid,sizeof(uint32_t));
				break;
			}
			case MUSE_ALLOC:{
					void* aux = message->content;
					uint32_t tam;
					memcpy(&tam,aux,sizeof(uint32_t));
					int res = muse_alloc(id_cliente,tam);
					if(res >0){
						send_status(muse_sock,OK,res);
						//log
					}else{
						send_status(muse_sock,ERROR,res);
						//log
					}
					break;
			}
			case MUSE_FREE:{
				uint32_t dir = *((uint32_t*) message->content);
				int res = muse_free(id_cliente,dir);
				if(res >=0){
					send_status(muse_sock,OK,res);
				}else{
					send_status(muse_sock,ERROR,-1);
				}
				break;
			}
			case MUSE_GET:{
				uint32_t src;
				size_t n;
				void*aux=message->content;
				memcpy(&src,aux,sizeof(uint32_t));
				aux+=sizeof(uint32_t);
				memcpy(&n,aux,sizeof(size_t));
				void* res = malloc(n);
				muse_get(id_cliente,res,src,n);
//				printf("%i !!!!!!!!!!!!!",*((int*)res));
				if(res != NULL){
					send_message(muse_sock,OK,res,n);
					free(res);
				}else{
					send_status(muse_sock,ERROR,-1);
				}
				break;
			}
			case MUSE_CPY:{
				uint32_t dst;
				size_t n;
				void* src;
				void* aux = message->content;
				memcpy(&dst,aux,sizeof(uint32_t));
				aux+=sizeof(uint32_t);
				memcpy(&n,aux,sizeof(size_t));
				aux+=sizeof(size_t);
				src = malloc(n);
				memcpy(src,aux,n);
				printf("%i !!!!!!!!!!!!!",*((int*)src));
				int res = muse_cpy(id_cliente,dst,src,n);
				if(res == -1){
					send_status(muse_sock,ERROR,-1);
				}else{
					send_status(muse_sock,OK,0);
				}
				free(src);
				break;
			}
			case MUSE_MAP:{
				char* path;
				size_t length;
				int flags;
				int len;
				void*aux = message->content;
				memcpy(&len,aux,sizeof(int));
				aux+=sizeof(int);
				path = malloc(len+1);
				memcpy(path,aux,len);
				path[len] = '\0';
				aux+=strlen(path);
				memcpy(&length,aux,sizeof(size_t));
				aux+=sizeof(size_t);
				memcpy(&flags,aux,sizeof(int));
				int res = muse_map(id_cliente,path,length,flags);
				if(res <0){
					send_status(muse_sock,ERROR,-1);
				}else{
					send_status(muse_sock,OK,res);
				}
				break;
			}
			case MUSE_SYNC:{
				uint32_t addr;
				size_t len;
				void* aux = message->content;
				memcpy(&addr,aux,sizeof(uint32_t));
				aux+=sizeof(uint32_t);
				memcpy(&len,aux,sizeof(len));
				int res = muse_sync(id_cliente,addr,len);
				if(res >=0){
					send_status(muse_sock,OK,0);
				}else{
					send_status(muse_sock,ERROR,-1);
				}
				break;
			}
			case MUSE_UNMAP:{
				uint32_t dir;
				void* aux = message->content;
				memcpy(&dir,aux,sizeof(uint32_t));
				int res = muse_unmap(id_cliente,dir);
				if(res >=0){
					send_status(muse_sock,OK,0);
				}else{
					send_status(muse_sock,ERROR,-1);
				}
				break;
			}
			case MUSE_CLOSE:{
				int res = muse_close(id_cliente);
				executing = 0;
				break;
			}
			case NO_CONNECTION:
				log_info(logger, "CLIENTE DESCONECTADO");
				free_t_message(message);
				pthread_exit(NULL);
				return NULL;
				break;
			case ERROR_RECV:
				free_t_message(message);
				log_info(logger, "ERROR COMUNIACIÓN");
				pthread_exit(NULL);
				return NULL;
				break;
			default:
				break;
			}
			free_t_message(message);
	}
	return NULL;
}


void init_muse_server() {
	listener_socket = init_server(PUERTO);
	log_info(logger, "Servidor levantado!!! Eschuchando en %i",PUERTO);
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


void inicializarLogger(char* path){
	char* nombre = string_new();
	string_append(&nombre,path);
	string_append(&nombre,".log");
	logger = log_create(nombre,"muse",1,LOG_LEVEL_TRACE);
	free(nombre);
}

void inicializarEstructuras(char* ruta){
	posicionInicialMemoria = malloc(TAMANIO_MEMORIA);
	espacioDisponible = TAMANIO_MEMORIA + TAMANIO_SWAP;
	CANT_PAGINAS_MEMORIA = TAMANIO_MEMORIA / TAMANIO_PAGINA;
	CANT_PAGINAS_SWAP = TAMANIO_SWAP / TAMANIO_PAGINA;
	listaMapeos = list_create();
	listaProgramas = list_create();
	punteroClock = 0;
	inicializarMemoriaVirtual(ruta);
	inicializarBitmap();
	inicializarSemaforos();
}

void visualizarBitmap(){
	for(uint32_t i = 0; i < CANT_PAGINAS_MEMORIA; i++){
		BitMemoria* bit = list_get(bitmap->bits_memoria,i);
		printf("[%d]: bit->esta_ocupado [%d]\n",i,bit->esta_ocupado);
		printf("[%d]: bit->pos [%d]\n",i,bit->pos);
		printf("[%d]: bit->bit_modificado [%d]\n",i,bit->bit_modificado);
		printf("[%d]: bit->bit_uso [%d]\n",i,bit->bit_uso);
		puts("-------------------------------------------");
	}

//	for(uint32_t i = 0; i < CANT_PAGINAS_SWAP; i++){
//		BitSwap* bit = list_get(bitmap->bits_memoria_virtual,i);
//		printf("[%d]: bit->esta_ocupado [%d]\n",i,bit->esta_ocupado);
//		printf("[%d]: bit->pos [%d]\n",i,bit->pos);
//	}
}

void inicializarBitmap(){
	bitmap = malloc(sizeof(Bitmap));
	bitmap->tamanio_memoria = CANT_PAGINAS_MEMORIA;
	bitmap->bits_memoria = list_create();
	for(uint32_t i = 0; i < CANT_PAGINAS_MEMORIA; i++){
		BitMemoria* bit = malloc(sizeof(BitMemoria));
		bit->esta_ocupado = false;
		bit->pos = i;
		bit->bit_modificado = false;
		bit->bit_uso = false;
		list_add(bitmap->bits_memoria,bit);
	}

	bitmap->tamanio_memoria_virtual = CANT_PAGINAS_SWAP;
	bitmap->bits_memoria_virtual = list_create();
	for(uint32_t i = 0; i < CANT_PAGINAS_SWAP; i++){
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
	printf("Ruta Swap: %s\n",rutaSwap);
	//log_info(logger,"Ruta area Swap = %s",rutaSwap);
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
			puts("F");
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
				puts("Creando segmento...");
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
			puts("Buscando si hay espacio para alocar...");
			Segmento* segmentoEncontrado = obtenerSegmentoParaOcupar(listaSegmentos,tamanio + sizeof(HeapMetadata));
			if(segmentoEncontrado != NULL){
				puts("Encontré espacio");
				// me pongo a buscar el espacio disponible
				InfoHeap* ultimoInfoHeap = ultimoElemento(segmentoEncontrado->status_metadata);
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

				InfoHeap* siguienteHeap = malloc(sizeof(InfoHeap));

				siguienteHeap->direccion_heap = numPaginaFinal * TAMANIO_PAGINA + offsetFinal;
				siguienteHeap->espacio = heapFinal->tamanio;
				siguienteHeap->libre = true;
				siguienteHeap->indice = heapConEspacio->indice + 1;

				// lo agrego al final by: frongi wtf
				list_add_in_index(segmentoEncontrado->status_metadata, heapConEspacio->indice+1, siguienteHeap);

				free(heapInicial);
				free(heapFinal);

			} else { // no encontró ningun segmento, hay que ver si se puede agrandar o crear otro //acá tambien podria ir lo de liberadas
				puts("No encontré :(");
				Segmento* ultimoSegmento = obtenerUltimoSegmento(listaSegmentos);
				if(ultimoSegmento->es_mmap){
					// acá no puedo agrandar asique creo otro segmento xd
					// ¿delegar funcion crear segmento? yo diria que no kent
					puts("Creando otro segmento porque habia un mmap");
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
					puts("Toca agrandar");
					InfoHeap* ultimoInfoHeap = ultimoElemento(ultimoSegmento->status_metadata);
					ultimoInfoHeap->libre = false;
					Pagina* ultimaPagina = ultimoElemento(ultimoSegmento->paginas);
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
						int numPaginaViejaFinal = techo((double)nuevoInfoHeaperino->direccion_heap / TAMANIO_PAGINA) - 1;

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
						theReturn = ultimoInfoHeap->direccion_heap + sizeof(HeapMetadata);
						//theReturn = ultimoSegmento->base_logica + sizeof(HeapMetadata) + nuevoInfoHeaperino->direccion_heap;
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

	if(heapEncontrado == NULL){
		puts("FFFFFFFFFFFFF");
		return -1;
	}

	HeapMetadata* newHM = malloc(sizeof(HeapMetadata));
	newHM->libre = true;
	newHM->tamanio = heapEncontrado->espacio;
	sustituirHeapMetadata(heapEncontrado,segmentoObtenido,newHM);
	free(newHM);
	merge(segmentoObtenido);

	for(int i = 0; i < list_size(segmentoObtenido->status_metadata); i++){
		InfoHeap* heap = list_get(segmentoObtenido->status_metadata,i);
		heap->indice=i;
	}

	log_info(logger,"Direccion %i liberada correctamente (segmento %s)",dir,id);
	return 0;

}

int muse_map(char* id, char* path, uint32_t length, uint32_t flag){
	int tamanioAMapear = techo((double)length / TAMANIO_PAGINA) * TAMANIO_PAGINA;
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
		segmentoNuevo->base_logica = obtenerBaseLogicaNuevoSegmento(listaSegmentos);
		segmentoNuevo->status_metadata = NULL;
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
		int cantidad_de_paginas = techo((double)length / TAMANIO_PAGINA);
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
		pthread_mutex_lock(&mut_mapeos);
		list_add(listaMapeos,mapeo);
		pthread_mutex_unlock(&mut_mapeos);
		printf("segmentoNuevo->base_logica = [%d]\n", segmentoNuevo->base_logica);
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
			int ultimaPag = techo((double)(dirAlSegmento + len) / TAMANIO_PAGINA) - 1;
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
			FILE* file = fopen(segmentoEncontrado->path_mapeo,"wb+");
			printf("segmentoEncontrado->path_mapeo = [%s]\n",segmentoEncontrado->path_mapeo);
			if(file != NULL){
				if(fseek(file,posInicial,SEEK_SET) == 0){
					puts("XD");
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

int muse_get(char* id, void* dst, uint32_t src, size_t n){
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

	int pagInicial = direccionAlSegmento / TAMANIO_PAGINA;
	int pagFinal = techo((double)(direccionAlSegmento + n) / TAMANIO_PAGINA) - 1;
	int offset = direccionAlSegmento % TAMANIO_PAGINA;

	if(segmentoEncontrado != NULL && segmentoEncontrado->tamanio - direccionAlSegmento >= n &&
	pagInicial * TAMANIO_PAGINA + offset + n <= segmentoEncontrado->tamanio){

		int cantidadPaginas = pagFinal - pagInicial + 1;
		int tamanioTotal = cantidadPaginas * TAMANIO_PAGINA;
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

		memcpy(dst, bloquesote + offset, n);
		printf("COSA: %s",(char*)dst);
		free(bloquesote);

	}
	//log_info(logger,"La cosa que pidieron pasar es: %s",(char*)dst);
	return 0;

}

int muse_cpy(char* id, uint32_t dst, void* src, size_t n){
	char* str = (char*) src;
	printf("COSA A ESAGFARHJUAH %s\n",str);
	t_list* listaSegmentos = obtenerListaSegmentosPorId(id);
	if(listaSegmentos == NULL || list_is_empty(listaSegmentos)){
		log_info(logger,"F por el programa %s que no existe o no tiene segmentos",id);
		return -1;
	}

	Segmento* segmentoEncontrado = obtenerSegmentoPorDireccion(listaSegmentos,dst);
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
				log_info(logger,"ya copié todo");
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
			if(punteroMarco != NULL){
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
		}

		return 0;
	}
}

int muse_close(char* idCliente){

    bool esProg(Programa* programa){
            return string_equals_ignore_case(programa->id,idCliente);
    }
    pthread_mutex_lock(&mut_listaProgramas);
    Programa* prog = list_find(listaProgramas,(void*)esProg);
    pthread_mutex_unlock(&mut_listaProgramas);
    if(prog!=NULL){
        pthread_mutex_lock(&mut_listaProgramas);
        list_remove_and_destroy_by_condition(listaProgramas,(void*)esProg,(void*)destruirPrograma);
        pthread_mutex_unlock(&mut_listaProgramas);
    }
    return 0;
}

//void recursiva(int num){
//	if(num == 0)
//		return;
//
//	uint32_t ptr = muse_alloc("prog1",4);
//	muse_cpy("prog1",ptr, &num, 4);
//	printf("\nNumCpy: [%d]\n", num);
//
//	recursiva(num - 1);
//
//	num = 0; // Se pisa para probar que muse_get cargue el valor adecuado
//	void* buffer = malloc(sizeof(int));
//	int get = muse_get("prog1",&num,ptr,sizeof(int));
//	printf("\nNumGet: [%d]\n",num);
//	//	mensaje_recibido1 = muse_get("prog1",5,strlen(mensaje1));
//	//muse_get("prog1",&num, ptr, 4);
//	//printf("\nNumGet: [%d]\n", num);
//	muse_free("prog1",ptr);
//}

char* pasa_palabra(int cod)
{
	switch(cod)
	{
	case 1:
		return strdup("No sabes que sufro?\n");
	case 2:
		return strdup("No escuchas mi plato girar?\n");
	case 3:
		return strdup("Cuanto tiempo hasta hallarte?\n");
	case 4:
	case 5:
		return strdup("Uh, haces mi motor andar\n");
	case 6:
		return strdup("Y mis cilindros rotar\n");
	default:
	{
		if(cod % 2)
			return strdup("Oh si\n");
		else
			return strdup("un Archivo de swap supermasivo\n");
	}
	}
}

void recursiva2(int num)
{
	if(num == 0)
		return;
	char* estrofa = pasa_palabra(num);
	int longitud = strlen(estrofa)+1;
	uint32_t ptr = muse_alloc("prog1",longitud);

	muse_cpy("prog1",ptr, estrofa, longitud);
	recursiva2(num - 1);
	muse_get("prog1",estrofa, ptr, longitud);

	printf("Estrofa = [%s]\n",estrofa);

	muse_free("prog1",ptr);
	free(estrofa);
	sleep(1);
}
