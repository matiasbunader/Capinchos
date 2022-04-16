
#include "servidor.h"

void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, char *puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete *crear_super_paquete(void)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

t_paquete *crear_paquete(void)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente, t_log *logger)
{
	close(socket_cliente);
}

int iniciar_servidor(char *puerto, char *ip, t_log *logger)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socket_servidor);
			continue;
		}
		break;
	}

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

//levanta una instancia de servidor tomando ip y puerto del archivo config, devolvuelve un socket en caso de exito o -1 en caso de error
int levantar_swamp()
{
	int server_fd = -1;
	t_config *config;
	char IP[10];
	if ((config = config_create("swamp.config")))
	{
		strcpy(IP, config_get_string_value(config, "IP"));
		server_fd = iniciar_servidor(config_get_string_value(config, "PUERTO"), IP, logger);
		log_info(logger, "swamp levantado");
	}
	else
		log_error(logger, "No se logro acceder al archivo de configuración de swamp");

	return server_fd;
}

int esperar_cliente(int socket_servidor, t_log *logger)
{
	struct sockaddr_in dir_cliente;
	size_t tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void *)&dir_cliente, (socklen_t *restrict)&tam_direccion);

	log_info(logger, "Se conecto la memoria!");

	return socket_cliente;
}

uint8_t recibir_operacion(int socket_cliente)
{
	uint8_t cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(uint8_t), MSG_WAITALL) != 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return ERROR;
	}
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log *logger)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

//podemos usar la lista de valores para poder hablar del for y de como recorrer la lista
t_list *recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while (desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
	return NULL;
}

//Devuelve el valor correspondiente a la clave key en el archivo filepath saliendo
//-informando por el logger- si no encuentra la clave o el archivo
char *get_value_from_key(char *filepath, t_log *logger, char *key)
{
	verfica_existencia_archivo(filepath, logger);
	t_config *config = config_create(filepath);
	if (!config_has_property(config, key))
	{
		log_error(logger, "no se encontró la clave %s", key);
		exit(EXIT_FAILURE);
	}
	int tamanio = strlen(config_get_string_value(config, key)) + 1;
	char *value = malloc(tamanio);

	strcpy(value, config_get_string_value(config, key));
	config_destroy(config);
	return value;
}

//verifica la existencia de archivo o sale informando por el logger el número de error(errno)
void verfica_existencia_archivo(char *archivo, t_log *logger)
{
	if (access(archivo, F_OK) == -1)
	{
		log_error(logger, "No se pudo abrir %s, error: %d",
				  archivo, errno);
		exit(EXIT_FAILURE);
	}
}

void escuchar(con_param *params)
{
	t_log *logger = params->logger;
	int cliente_fd = params->conexion;

	/*void iterator(char *value)
	{
		printf("%s\n", value);
	}

	t_list *lista;*/

	while (1)
	{
		uint8_t cod_op = recibir_operacion(cliente_fd);
		switch (cod_op)
		{
		case ESCRIBIR_PAGINA:
			escribir_pagina(cliente_fd, logger);
			break;
		case LEER_PAGINA:
			leer_pagina(cliente_fd, logger);
			break;
		case TIPO_ASIGNACION:
			recibir_tipo_asignacion(cliente_fd, logger);
			break;
		case ELIMINAR_CARPINCHO:
			matar_carpincho(cliente_fd);
			break;
		case ERROR:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			exit(EXIT_FAILURE);
			/*default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			exit(EXIT_FAILURE);*/
			//break;
		}
	}
}

void enviar_consola(t_log *logger, int conexion)
{
	void loggear(char *leido)
	{
		enviar_mensaje(leido, conexion);
		log_info(logger, leido);
	}

	_leer_consola_haciendo((void *)loggear);
}

char *leer_consola(t_log *logger)
{
	char *leido = readline("swamp>> ");
	log_info(logger, leido);
	return leido;
}

t_paquete *armar_paquete()
{
	t_paquete *paquete = crear_paquete();

	void _agregar(char *leido)
	{
		// Estamos aprovechando que podemos acceder al paquete
		// de la funcion exterior y podemos agregarle lineas!
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
	}

	_leer_consola_haciendo((void *)_agregar);

	return paquete;
}

void _leer_consola_haciendo(void (*accion)(char *))
{
	char *leido = readline("root>> ");

	while (strncmp(leido, "", 1) != 0)
	{
		accion(leido);
		free(leido);
		leido = readline(">");
	}

	free(leido);
}

void hablar(con_param *params)
{
	t_log *log = params->logger;
	int conexion = params->conexion;

	while (1)
	{
		enviar_consola(log, conexion);
	}
}

void escribir_pagina(int socket, t_log *logger)
{
	uint32_t carpincho, tamanio_datos;
	uint16_t pagina;

	uint8_t bandera = 1;
	if (recv(socket, &carpincho, sizeof(uint32_t), MSG_WAITALL) == 0)
	{
		log_error(logger, "ESCRIBIR_PAGINA: ERROR AL LEER CARPINCHO DE STREAM");
		bandera = 0;
	}
	if (recv(socket, &pagina, sizeof(uint16_t), MSG_WAITALL) == 0)
	{
		log_error(logger, "ESCRIBIR_PAGINA: ERROR AL LEER NUMERO DE PAGINA DE STREAM");
		bandera = 0;
	}
	if (recv(socket, &tamanio_datos, sizeof(uint32_t), MSG_WAITALL) == 0)
	{
		log_error(logger, "ESCRIBIR_PAGINA: ERROR AL LEER TAMANIO DE DATOS DE STREAM");
		bandera = 0;
	}
	void *datos = malloc(tamanio_datos);

	if (recv(socket, datos, tamanio_datos, MSG_WAITALL) == 0)
	{
		log_error(logger, "ESCRIBIR_PAGINA: ERROR AL LEER DATOS DE STREAM");
		bandera = 0;
	}
	if (bandera == 1)
		log_info(logger, "ESCRIBIR_PAGINA: SE LEYO DEL STREAM CARPINCHO(%d), PAGINA(%d), TAMANIO DE DATOS(%d), DATOS(%s)", carpincho, pagina, tamanio_datos, (char *)datos);
	//TODO:con el carpincho, pagina y datos persisto los ultimos en la particion que corresponda segun el tipo de asignacion y actualizo la lista de paginas si corresponde
}

void leer_pagina(int socket, t_log *logger)
{
	uint32_t carpincho;
	uint16_t pagina;

	uint8_t bandera = 1;
	if (recv(socket, &carpincho, sizeof(uint32_t), MSG_WAITALL) == 0)
	{
		log_error(logger, "LEER_PAGINA: ERROR AL LEER CARPINCHO DE STREAM");
		bandera = 0;
	}
	if (recv(socket, &pagina, sizeof(uint16_t), MSG_WAITALL) == 0)
	{
		log_error(logger, "LEER_PAGINA: ERROR AL LEER NUMERO DE PAGINA DE STREAM");
		bandera = 0;
	}
	if (bandera == 1)
		log_info(logger, "LEER_PAGINA: SE LEYO DEL STREAM CARPINCHO(%d), PAGINA(%d)", carpincho, pagina);
	//TODO:con el carpincho y pagina accedo a la particion, obtengo los datos y los devuelvo por el socket
}

void recibir_tipo_asignacion(int socket, t_log *logger)
{
	if (recv(socket, &tipo_asignacion, sizeof(uint8_t), MSG_WAITALL) == 0)
		log_error(logger, "LEER_TIPO_ASIGNACION: ERROR AL LEER TIPO DE ASIGNACION DE STREAM");
	else
		log_info(logger, "LEER_TIPO_ASIGNACION: SE ASIGNO TIPO %s", tipo_asignacion == ASIGNACION_FIJA ? "FIJA" : "GLOBAL");
}

void matar_carpincho(int socket)
{
	uint32_t *carpincho_id = malloc(sizeof(uint32_t));
	if (recv(socket, carpincho_id, sizeof(uint32_t), MSG_WAITALL) == 0)
	{
		log_error(logger, "MATAR_CARPINCHO: ERROR AL LEER ID DEL CARPINCHO DEL STREAM");
		free(carpincho_id);
	}
	else
		eliminar_carpincho(*carpincho_id);
}

t_asignacion obtener_asignacion()
{
	return tipo_asignacion;
}