#ifndef MATELIB_H_INCLUDED
#define MATELIB_H_INCLUDED

#include <stdint.h>
#include <commons/log.h>
#include <pthread.h>

//-------------------Type Definitions----------------------/

typedef struct mate_inner_structure {
    int pid;
    int conexion;
    t_log* logger;
    char* ip;
    char* puerto; 
} mate_inner_structure;

typedef struct mate_instance
{
    mate_inner_structure *group_info;
} mate_instance;

typedef char *mate_io_resource;

typedef char *mate_sem_name;

typedef int32_t mate_pointer;


//------------------General Functions---------------------/

/**
	* @NAME: mate_init
	* @DESC: Crea una instancia de  la matelib
	*/
int mate_init(mate_instance *lib_ref, char *config);

/**
	* @NAME: mate_close
	* @DESC: Finaliza la instancia de la matelib
	*/
int mate_close(mate_instance *lib_ref);


//-----------------Semaphore Functions---------------------/

/**
	* @NAME: mate_sem_init
	* @DESC: Inicializa el semáforo con el nombre sem. El argumento de value especifica el valor  inicial para el semáforo.
	*/
int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value);

/**
	* @NAME: mate_sem_wait
	* @DESC: Disminuye (bloquea) el semáforo de nombre sem.
	*/
int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem);

/**
	* @NAME: mate_sem_post
	* @DESC: Incrementa (desbloquea) el semáforo de nombre sem. 
    * Si, el valor del semáforo se vuelve mayor que cero, otro carpincho bloqueado en una llamada a mate_sem_wait se desbloqueará.
	*/
int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem);

/**
	* @NAME: mate_sem_destroy
	* @DESC: Destruye el semáforo de nombre sem
	*/
int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem);


//--------------------IO Functions------------------------/

/**
	* @NAME: mate_call_io
	* @DESC: Solicita el dispositivo de nombre io.
	*/
int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg);


//--------------Memory Module Functions-------------------/

/**
	* @NAME: mate_memalloc
	* @DESC: Asigna la memoria solicitada (size) y devuelve la direccion de memoria. 
	*/
mate_pointer mate_memalloc(mate_instance *lib_ref, int size);

/**
	* @NAME: mate_memfree
	* @DESC: Libera la memoria previamente asignada en la direccion de memoria addr.
	*/
int mate_memfree(mate_instance *lib_ref, mate_pointer addr);

/**
	* @NAME: mate_memread
	* @DESC: Obtiene la informacion de memoria guardada en la direccion origin.
	*/
int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size);

/**
	* @NAME: mate_memwrite
	* @DESC: Escribe en la direccion de memoria dest la información guardada en origin.
	*/
int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size);


//--------------   Otras   -------------------/


typedef enum
{   
    INIT=0,
    MEMALLOC,
    MEMFREE,
    MEMREAD,
    MEMWRITE,
	CLOSE=6,
    SEM_INIT,
    SEM_WAIT,
    SEM_POST,
    SEM_DESTROY,
    CALL_IO,
    PAQUETE,
    MENSAJE,	
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

typedef enum
{
	MATE_WRITE_FAULT=-7,
    MATE_READ_FAULT=-6,
    MATE_FREE_FAULT=-5,
    STATUS_SEM_INEXISTENTE=-4,
    MATE_CONNECTION_FAULT=-2,
	STATUS_ERROR=-1,
    STATUS_SUCCESS=0,
}mate_errors;

typedef struct{
	int pid;
	int status;
}t_msj_init;

typedef enum{
	MATELIB,
    KERNEL,
}e_remitente;

 int crear_conexion(char *ip, char* puerto);
 t_buffer* crear_tbuffer();
 void buffer_pack(t_buffer* buffer, void* stream_to_add, int size);
 void liberar_buffer(t_buffer* buffer);
 void buffer_pack_string(t_buffer* buffer, char* string_to_add);
 t_log* iniciar_logger(void);
 int recibir_status( int socket );
 char* status_to_string(int valor);
 t_msj_init* buffer_unpack_init(t_buffer* buffer);
 void recibir_buffer(int sockfd, t_buffer* buffer);
 void buffer_unpack(t_buffer* buffer, void* dest, int size);
 void enviar_mensaje(op_code codigo_operacion, t_buffer* buffer, int socket);
 int  solicitar_init(mate_instance *lib_ref);
 void inicializar(mate_instance *lib_ref, char *path_config);

#endif