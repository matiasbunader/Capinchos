#ifndef KERNEL_H
#define KERNEL_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>
#include "shared_utils.h"
#define MAX_CONEXIONES 100

int server_fd;
int pid_counter;
t_log* logger_aux;
int tiempo;
int  cantidadCPU;
double estimacion_inicial;
double alfa;
char* algoritmo;
t_config *config;


sem_t sem_multiprogramacion;
sem_t sem_new;
sem_t sem_ready;


t_list *ESTADO_NEW;
t_list *ESTADO_READY;
t_list *ESTADO_SUSPENDED_READY;
t_list *ESTADO_BLOCKED;
t_list *ESTADO_SUSPENDED_BLOCKED;
t_list *ESTADO_EXEC;
t_list *ESTADO_EXIT;
t_list *DISPOSITIVOS_IO;
t_list *SEMAFOROS;

pthread_t *threads_CPU;
pthread_t hiloEscucha;
pthread_t hiloPlaniLargoPlazo;
pthread_t hiloPlaniCortoPlazo;
pthread_t hilo_chequeo_deadlock;
 

pthread_mutex_t mutex_pid_counter;
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_suspended_ready;
pthread_mutex_t mutex_blocked;
pthread_mutex_t mutex_suspended_blocked;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_tiempo;
pthread_mutex_t mutex_dispositivosIO;
pthread_mutex_t mutex_semaforos;



typedef enum {
	NEW=0,READY, SUSPENDED_READY,BLOCKED,SUSPENDED_BLOCKED,  EXEC, EXIT, 
} t_status;

typedef struct {
	int pid;
	t_status estado;
   	int conexion; //Conexion con el carpicho abierta(matelib)
	double rafaga_estimada;
	//double rafaga_estimada_anterior;
	double rafaga_real_anterior;
	double RR; //Response Ratio lo calculo cada vez que alguien entra en EXEC.
	char* ultimo_tiempo_inicio_exec; //Para calcular proxima rafaga.
	char* ultimo_tiempo_inicio_ready; //Para calcular W: espera.
	//time_t ultimo_tiempo_inicio_exec; //Para calcular proxima rafaga.
	//time_t ultimo_tiempo_inicio_ready; //Para calcular W: espera.
	t_list* recursos_asignados;
	bool envios_pendientes;
	//e_status status_pendiente_de_enviar;
	t_buffer* envio_pendiente;
} t_pcb;

typedef struct{
	char *nombre;
	int cantidad;
}t_sem_uso;

typedef struct{
	char * nombre;
	t_list* carpinchos_involucrados;
}t_deadlock;


typedef struct{
	char *nombre;
	pthread_mutex_t mutex_sem;
	t_queue* queue_espera;
    t_list* carpinchos_en_uso;
	int valor;
}t_semaforo;

typedef struct{
	char* nombre;
	int duracion;
	sem_t* sem_carpincho_en_cola;
	sem_t* sem_termino_de_ejecutar;
	t_queue* queue_carpinchos_en_espera;

}t_dispositivoIO;

typedef struct{
	int pid;
	char*nombre;
	int value;
}t_msj_seminit;

typedef struct{
	int pid;
	int add;
}t_msj_memfree;

typedef struct{
	int pid;
	int origin;
	int size;
}t_msj_memread;

typedef struct{
	int pid;
	int tamanio;
}t_msj_memalloc;

typedef struct{
	int pid;
	char*nombre;
}t_msj_semwait;

typedef struct{
	int pid;
	char*nombre;
}t_msj_sempost;

typedef struct{
	int pid;
	int dest;
	char* origin;
}t_msj_memwrite;

typedef struct{
	int pid;
	char*nombre;
}t_msj_semcallio;

typedef struct{
	int pid;
	char*nombre;
}t_msj_semdestroy;

typedef struct{	
	e_remitente remitente;
}t_msj_init;

#endif