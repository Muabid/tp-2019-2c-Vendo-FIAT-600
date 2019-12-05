#ifndef UTILS_H_
#define UTILS_H_

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

t_list* obtenerListaSegmentosPorId(char* idPrograma);
t_list* obtenerMapeoExistente(char* path,int tamanio);
Segmento* segmentoAlQuePertenece(t_list* listaSegmentos, uint32_t direccion);
void paginasMapEnMemoria(int direccion, int tamanio, Segmento* segmentoEncontrado);
bool tienePaginasNoCargadasEnMemoria(Segmento* segmento, int pagInicial, int pagFinal);
void* obtenerPunteroAMarco(Pagina* pag);
BitMemoria* asignarMarcoNuevo();
BitMemoria* buscarBitLibreMemoria();
void paginasMapEnMemoria(int direccion, int tamanio, Segmento* segmentoEncontrado);
bool tienePaginasNoCargadasEnMemoria(Segmento* segmento, int pagInicial, int pagFinal);
void unmapear(Segmento* segmento, char* idPrograma);
void destruirMapeo(Mapeo* mapeo);
void destruirPagina(Pagina* pag);
void* generarRelleno(int relleno);
Segmento* obtenerSegmentoPorDireccion(t_list* listaSegmentos, uint32_t direccion);
int techo(double d);
uint32_t calcularFramesNecesarios(uint32_t tam);
Segmento* obtenerSegmentoParaOcupar(t_list* listaSegmentos,uint32_t tamanio);
InfoHeap* obtenerHeapConEspacio(Segmento* segmento, uint32_t tamanio);
Segmento* obtenerUltimoSegmento(t_list* listaSegmentos);
uint32_t obtenerBaseLogicaNuevoSegmento(t_list* listaSegmentos);
void sustituirHeapMetadata(InfoHeap* heapLista, Segmento* seg, HeapMetadata* new);
void merge(Segmento* segmentoEncontrado);

BitSwap* buscarBitLibreSwap();
BitSwap* pasarASwap(BitMemoria* bit);
Pagina* buscarPaginaSegunBit(BitMemoria* bit);
BitMemoria* buscarBit_0_0();
BitMemoria* buscarBit_0_1();
BitMemoria* ejecutarClockModificado();
BitMemoria* ejecutarSegundaVueltaClock();


#endif /* UTILS_H_ */
