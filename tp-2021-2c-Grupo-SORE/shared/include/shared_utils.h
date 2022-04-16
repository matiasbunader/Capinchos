#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>


typedef enum
{   
    INIT=0,
    MEMALLOC,
    MEMFREE,
    MEMREAD,
    MEMWRITE,
    SUSPENDER,
    CLOSE,
    SEM_INIT,
    SEM_WAIT,
    SEM_POST,
    SEM_DESTROY,
    CALL_IO,
    PAQUETE,
    MENSAJE,
}op_code;

typedef enum
{   MATE_WRITE_FAULT=-7,
    MATE_READ_FAULT=-6,
    MATE_FREE_FAULT=-5,
    STATUS_SEM_INEXISTENTE=-4,
    DEVICE_NOT_FOUND=-3,
    MATE_CONNECTION_FAULT,
	STATUS_ERROR=-1,
    STATUS_SUCCESS=0,   
}e_status;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

// typedef struct
// {
// 	op_code codigo_operacion;
// 	t_buffer* buffer;
// } t_paquete;

typedef enum{
	MATELIB,
    KERNEL,
}e_remitente;



void recibir_buffer(int sockfd, t_buffer* buffer);
t_buffer* crear_tbuffer();
int iniciar_servidor(char* ip, char* port);
int esperar_cliente(int);
void recibir_mensaje(int);
int recibir_operacion(int);




int crear_conexion(char* ip, char* puerto);
//void enviar_mensaje(char* mensaje, int socket_cliente);
void liberar_conexion(int socket_cliente);
t_log *logger;

void buffer_pack(t_buffer* buffer, void* stream_to_add, int size);
void buffer_unpack(t_buffer* buffer, void* dest, int size);

//t_paquete* crear_paquete(op_code codigo);
//void eliminar_paquete(t_paquete* paquete);
//void agregar_a_paquete(t_paquete* paquete, void* valor, uint32_t tamanio);
//void enviar_paquete(t_paquete* paquete, int socket_cliente);
//void* serializar_paquete(t_paquete* paquete, int bytes);

#endif