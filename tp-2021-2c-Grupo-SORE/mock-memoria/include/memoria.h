#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <stdbool.h>
#include <pthread.h>
#include "shared_utils.h"
#define MAX_CONEXIONES 100
#define IP "127.0.0.1"
#define PUERTO "5001"

void inicializar();
void iniciar_logger(void);
void* escucharConexiones();
void* handler(void* socketConectado);
e_remitente  buffer_unpack_init( buffer);
void procesar_init(void* buffer, int socket);
void procesar_memalloc(void* buffer, int socket);
void procesar_memfree(t_buffer* buffer, int socket);
int buffer_unpack_memalloc(t_buffer *buffer);
void enviar_status(e_status status, int socket );
const char* status_to_string(int valor);
int server_fd;
pthread_t hiloEscucha;
t_log *logger;
char* leido;

#endif