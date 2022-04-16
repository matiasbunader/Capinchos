/*
 *
 *     Author: yamil alejandro lopez
 */

#ifndef SERVIDOR_H_
#define SERVIDOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <pthread.h>

#include "swap.h"

typedef enum
{
	ESCRIBIR_PAGINA = 1,
	LEER_PAGINA,
	TIPO_ASIGNACION,
	ELIMINAR_CARPINCHO,
	ERROR,
	MENSAJE,
	PAQUETE
} op_code;

typedef struct
{
	int size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

typedef struct
{
	t_log *logger;
	int conexion;
} con_param;

void *recibir_buffer(int *, int);

int iniciar_servidor(char *, char *, t_log *);
int esperar_cliente(int, t_log *);
t_list *recibir_paquete(int);
void recibir_mensaje(int, t_log *);
uint8_t recibir_operacion(int);
void escuchar(con_param *);
void verfica_existencia_archivo(char *, t_log *);
char *get_value_from_key(char *, t_log *, char *);
int crear_conexion(char *ip, char *puerto);
void enviar_mensaje(char *mensaje, int socket_cliente);
t_paquete *crear_paquete(void);
t_paquete *crear_super_paquete(void);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void liberar_conexion(int socket_cliente, t_log *);
void eliminar_paquete(t_paquete *paquete);
void enviar_consola(t_log *logger, int conexion);
char *leer_consola(t_log *logger);
t_paquete *armar_paquete();
void _leer_consola_haciendo(void (*accion)(char *));
//void hablar(t_log *,int);
void hablar(con_param *);

int levantar_swamp();
t_asignacion obtener_asignacion();
void recibir_tipo_asignacion(int, t_log *);
void matar_carpincho(int);
void escribir_pagina(int, t_log *);
void leer_pagina(int, t_log *);


#endif /* SERVIDOR_H_ */
