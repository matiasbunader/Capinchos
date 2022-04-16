/*
 * utils.h
 *
 *  Created on: 14 sep. 2021
 *      Author: utnso
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <time.h>
#include <string.h>
#include <signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include <pthread.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/config.h>
#include<commons/string.h>


#include<math.h>


int socket_cliente;

void* memoriaCompleta;
t_list* listaDeProcesos;
t_list* listaDeMarcos;
t_config* configMemoria;
t_log* logger;
t_list* globalLRUCLOCK;
t_list* registroTLB;
uint32_t marcosMaximosSwamp;

uint32_t posicionClock;
uint32_t tamanioPagina;
uint32_t tamanioMemoria;
uint32_t marcosPorCarpincho;
uint32_t senaladorMarcosDisponibles;
char* algoritmoRemplazoTLB;
uint32_t cantidadEntradasTLB;
char* tipoSwap;
char* tipoAsignacion;
char* IPswamp;
char* puertoSwamp;
char* IPKernel;
char* puertoKernel;
bool escritura;
pthread_t hiloServer;
pthread_t metricas;
int conexionSwamp;
int TLBhitGlobal;
int TLBmissGlobal;

typedef struct{
	uint32_t id_carpincho;
	uint16_t pagina;
	uint32_t marco; 
}referenciaGlobalClockLRU;

typedef struct{
	uint32_t size;
	uint32_t marco;
	uint8_t bitModificacion;
	uint8_t bitPresencia;
	uint8_t bitUso;


}Pagina;

typedef struct{
	uint16_t pagina;
	uint32_t id;
	uint32_t marco;
	uint32_t entrada;
}TLB;

typedef struct{
	uint32_t pid;
	uint32_t size;


}Memalloc;

typedef struct{
	uint32_t pid;
	uint32_t dirLog;
	uint32_t size;
}Memread;

typedef struct{
	uint32_t pid;
	uint32_t dirLog;
	uint32_t tamanioCont;
	char* contenido;
}Memwrite;

typedef struct{
	uint32_t pid;
	uint32_t dirLog;
}MemFree;

typedef struct {
	uint32_t id_proceso;
	uint32_t tamanio;
	uint32_t hit;
	uint32_t miss;
	t_list* localLRUCLOCK;
	int referencioClock;
	t_list* tablaDePaginas;
}AdmProceso;

typedef struct{
	bool esLibre;
	void* marco;
}Marco;

typedef struct{
	uint32_t prevAlloc;
	uint32_t nextAlloc;
	uint8_t esLibre;
}__attribute__((packed)) HeapMetadata;


typedef enum{
	MATE_INIT,
	MATE_MEMALLOC,
	MATE_MEMFREE,
	MATE_MEMREAD,
	MATE_MEMWRITE,
	MATE_SUSPENDER,
	CLOSE,
	ESCRITURA,
    LECTURA,
	ASIGNACION,
	CONSULTARESERVA,
	ELIMINARCARPINCHO,
    LIBERARMARCO,
	CREARCARPINCHO,
	SALIR
}Cod_op;

typedef enum{
	MATELIB,
    KERNEL,
}e_remitente;

typedef struct mate_instance
{
    void *group_info;
} mate_instance;

typedef char *mate_io_resource;

typedef char *mate_sem_name;

typedef int32_t mate_pointer;

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;




void* iniciar_servidor(void);
void esperar_cliente(int);
void* armado_a_enviar(uint8_t codigo_operacion, t_buffer* buffer) ;
void recibir_operacion(int*);
void procesar_mensaje(uint32_t codop, int socket);
void liberar_conexion(int socket_cliente);
void liberarBuffer(t_buffer* buffer);
void* recibir_buffer( int socket_cliente);
int crear_conexion();
t_paquete* crear_paquete(op_code code);

mate_pointer deserializarPuntero(void*buffer);

Memalloc* deserializarMemalloc(void* buffer);

Memread* deserializarMemread(void* buffer);

Memwrite* deserializarMemwrite(void* buffer);

MemFree* deserializarMemFree(void* buffer);

void* serializarPageFault(uint32_t pidVic,uint16_t paginaVict,void* marco,int*bytes);

void* serializarSolicitudPaginaPageFault(uint16_t pagina,uint32_t pidSo,int*bytes);

void enviarVictimaPageFault(uint32_t pidVictima,uint16_t paginaVictima, void* marco);

void enviarSwampAlloc(void* alloc,uint32_t size,uint32_t pid,uint16_t pagina);

void recuperacionPaginaFaltante(uint32_t pid,uint16_t pagina);

void* serializarTipoAsignacion(uint8_t tipo,int*bytes);

void retornarTodasLasPaginasFaltantes(uint32_t alloc,t_list* paginas,uint32_t pid);

void* solicitarPaginaPageFault(uint16_t paginaSolicitante,uint32_t pidSolicitada);

void* analizarTipoPageFault(uint16_t pagVictima, uint32_t pidVictima,
		uint16_t paginaSolicitada, uint32_t pidSolicitada, void* aSwapear);

void enviarTipoAsignacion(char* tipoAsignacion);



void* serializarConsultaReserva(uint32_t id, uint16_t pagina, uint32_t reserva,
		int*bytes);

uint32_t enviarConsultaReserva(uint32_t id, uint16_t pagina, uint32_t reserva) ;

void enviarContenidoRespuesta(int socket,void*contenido,uint32_t size,uint32_t respuesta);

void* serializarContenidoRespuesta(void*contenido,uint32_t size,uint32_t respuesta, uint32_t*bytes);

void realizarEnvioLiberacioMarco(uint32_t pid,uint16_t pagina);
//-----------------------------------Funciones Memoria----------------------
void inicializarVariables();

void finalizarVariables();

AdmProceso* crearNuevocarpincho(uint32_t);

int posibleAsignacion();

//-------------------------------------------------Funciones Memalloc-------------------------------

t_list* crearListaLRU();

uint32_t obtenerMarcoLibreFijo(t_list* listaLRU);

void realizarMemalloc(void*buffer,int socket);

uint32_t primeraAsignacion(int size, AdmProceso* proceso);

HeapMetadata* crearHeapMetadata();

void IniciarProcesoMemoria(void *buffer, int socket);

int obtenerMarcoLibre();

AdmProceso* buscarCarpincho(uint32_t pid);

void iniciarListaMarcos();

uint32_t asignarMasPaginas(int size, int tamanioActual,AdmProceso* proceso);

void* crearAlloc(uint32_t size, uint32_t dirLog) ;

void mostrarListaCLOCKLRU(t_list *listLRU);
//-------------------------------------------------Funciones Memread--------------------------------

void* juntarPaginasParaAsignar(t_list* paginas, uint32_t tamanioTotal,uint32_t size);

void RealizarLecturaPaginas(void *buffer,int socket);

void* retornarContenidoAllocLectura(uint32_t dirLog, t_list*paginas,uint32_t id,uint32_t tamanioCont);

int cantidadPaginas(int size);

void* retornarContenido(t_list* paginas, uint32_t tamanioTotal,int *tamanioEnviar,uint32_t dirLog);

//=================================================Funciones memwrite========================
bool escribirAllocConListaTLB(uint32_t id, uint32_t alloc,	uint32_t tamanioContenido, void* contenido);

void*juntarContenidoMemoria(t_list* paginas,uint32_t tamanioTota);

void escribirEnMemoria(void* aEscribir, uint32_t tamanioTotal,uint32_t tamanioCont, char*contenido,uint32_t dirLog);

void pisarEnMemoriaPrinsipal(void* contenido, t_list* paginas);

void realizarEscrituraMemoria( void*buffer,int socket);

bool verificarPaginasPresentesAlloc(uint32_t dirLog, t_list*paginas,
		uint32_t id, uint32_t tamanioContenido, void* contenido);
//=================================================Funciones memfree========================

void realizarMemFree(void* buffer,int socket);

uint32_t memfree(uint32_t dirLog, AdmProceso *proceso);

uint32_t obtenerResto(uint32_t dirLog);

void recorrerHeaps(t_list* paginas);

uint16_t obtenerDeterminadaPagina(uint32_t dirLog);

void liberarMarcoLibre(HeapMetadata *actual, AdmProceso*proceso, uint32_t dirLog);

void* buscarMarcoDirLog(uint32_t dirLog,t_list*paginas);


void verificarNotificacionLiberacionMarco(uint32_t pid, uint16_t pagina);

//===============================================Funciones allocar en alloc libre================================
uint32_t obtenerDesplazamiento(uint32_t dirLog);

uint32_t allocarPrimero (int size,t_list*paginas,void* reservado);

uint32_t hayParaAllocar(int size,t_list*paginas,void* reservado);

void allocarEnAllocLibre(int size, AdmProceso *proceso, uint32_t alloc);

void* ptr_marco(uint32_t dirLog,t_list *paginas);

void modificarPaginaYliberarMarco(AdmProceso*proceso, uint16_t pagina);

uint32_t hayAllocLibre(int size,uint32_t pid, t_list*paginas);

bool hayMarcosLibres();

Pagina* crearPagina();
//================================================Funciones Remplazo sustitucion========================

int encontrarUnaVictimaLocalLRU(AdmProceso*proceso);

void actualizadorVictimasLRU(Pagina* pagina,uint32_t id,uint32_t pos,t_list*localLRU);

int encontrarUnaVictimaGlobalLRU(uint16_t *paginaVictima,uint32_t*pidVictima);

void realizarSustitucionPaginaGlobal(AdmProceso *proceso, uint32_t pagina);

void realizarSustitucionPagina(AdmProceso *proceso, uint32_t pagina,uint32_t cantidad);

void realizarSustitucionPaginaLocal(AdmProceso *proceso, uint32_t pagina);
referenciaGlobalClockLRU *encontrarUnaVictimaGlobalClockM();

Pagina* buscarVictimaGlobal(int *marco);

int encontrarUnaVictimaLocalClockM(AdmProceso*proceso);

void* realizarSwapGlobal(t_list*paginas,uint16_t *paginaVictima,uint32_t *pidVictima, uint16_t pagina,uint32_t id);

void* realizarSwapLocal(AdmProceso* proceso, uint16_t *paginaVictima,uint32_t *pidVictima,uint16_t pagina);

bool paginaPresenteEnMemoria(t_list* paginas,int pagina);

bool direccionLogicaValida( AdmProceso* proceso,uint32_t dirLog);

void actualizarSelectorVictima(Pagina* actualizar, AdmProceso*proceso,
		uint32_t dirLog, bool escritura);

void iniciarAdministradorDeVictimasLRU(Pagina* pag,int pos,uint32_t id,t_list* posicionesLRU);

void* juntarContenidoPaginasAlloc(t_list* paginas,uint32_t alloc,int* tamanio);

void* juntarContenidoParaSwampAlloc(t_list*paginas, uint32_t alloc,uint32_t *tamanioAlloc,uint16_t* pagina);

uint32_t posicionListaGlobalCLOCKLRU();

referenciaGlobalClockLRU* analizarReferenciaGlobal(uint32_t pos);

Pagina* analizarPaginaClock(t_list* local,uint32_t pagina,t_list*paginas);

void pisarDesdePaginaPagina(uint32_t paginaInicial,uint32_t paginaFinal,t_list*paginas,void* aPisar);

HeapMetadata* entreDosPaginas(uint32_t dirLog,AdmProceso*proceso);

HeapMetadata* controlarHeapPartido(uint32_t dirlog,uint32_t marco,AdmProceso* proceso);

void actualizarParametrosPAraAlgoritmos(Pagina *pagina, AdmProceso *proceso);

Pagina* buscarPaginaRelacionadaMarco(AdmProceso*proceso,uint32_t posicion,uint32_t *paginaPos);

void mostrarPaginasTotales(t_list *paginas,uint32_t id);

void concretarElPageFault(uint32_t pid,uint32_t dirLog);

void mostrarListaLRU(t_list * listLRU);

void* recorrerHeapsModificar(t_list* paginas,uint32_t resta);

uint32_t obtenerMarcoLibreFIJO();

uint32_t controlPosicionClock(uint32_t posicion,uint32_t limite);

void comprobarAplicacionDeSustitucion(AdmProceso* proceso,uint32_t cantidadDePagina);
//--------------------------------Dumping------------------------------
void *sistemaDeMetricas();

void dumpMemoria();

char* estadoPagina(bool estado);

int cantidadEntradas();

void tlbHitandMiss();

int metricasHitTLB(AdmProceso* proceso);

int metricasMissTLB(AdmProceso* proceso);


//----------------------------------Funciones sobre la TLB--------------------------------------
void nuevaEntradaTLB(uint16_t pagina, uint32_t marco,uint32_t id);

void removerTLBvictima(uint16_t numeroPagina,uint32_t pid);

void removerTodasLasEntradas(t_list*listaTLB);

int paginasPresentes(void* elem);

void mostrarListaTLBs();

TLB* buscarTLBvictimaFIFO(uint32_t pid);

TLB* buscarTLBvictimaLRU(uint32_t pid);

void verificarEnvioPaginaRemplazoTLB(uint16_t pagina,uint32_t id,t_list* paginas);

void  aplicarAlgoritmoRemplazoTLB(uint32_t id, uint32_t cantidadRemplazos,t_list*paginas,t_list* localLRU);

TLB *devolberTLBconPagina(uint16_t pagina, uint32_t pid);

TLB *devolberTLBconAMarco(uint32_t marco);

void removerTodasLasEntradasSuspencion(t_list*listaTLB,uint32_t id);

void* verificarPaginasAllocConTLBParaLectura(uint32_t id, uint32_t alloc,uint32_t size,
		uint32_t *tamanioCont);

void aplicarAlgoritmoRemplazoTLBGlobal( uint32_t cantidadRemplazos);

TLB* actualizarListaTLBporAlgoritmo(uint32_t marco);

void controlarCreacionTLB(uint16_t pagina, uint32_t marco, uint32_t id);

bool verificarDuplicadoMarco(uint32_t marco);
//=============================================Suspencion===================================
AdmProceso* removerCarpincho(uint32_t id);

void realizarSuspencionProceso(uint32_t id,int socket);

void liberarCarpinchoMemoria(uint32_t id);

#endif /* UTILS_H_ */
