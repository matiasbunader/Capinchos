#include "shared_utils.h"


int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
			server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen)
			== -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
}


t_buffer* crear_tbuffer() {
	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->size = 0;
	buffer->stream = NULL;

	return buffer;
}


void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}


int iniciar_servidor(char* ip,char* port) {
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &hints, &servinfo);
 
	// Creamos el socket de escucha del servidor
	if ((socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype,
			servinfo->ai_protocol)) == -1) {
		//printf("\nNo se creo correctamente el servidor\n");
	} else {
		//printf("\nSe creo correctamente el servidor\n");
	}
	// Asociamos el socket a un puerto
	if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		close(socket_servidor);
		//printf("No se creo correctaamente el servidor");

	} else {
		//printf("\nSe realizo el bind correctamente\n");
	}
	// Escuchamos las conexiones entrantes

	if (listen(socket_servidor, SOMAXCONN)) {
		//printf("\nNo se creo correctamente el servidor\n");
	} else {
		//printf("\nEl servidor esta listo para escuchar\n");
	}

	freeaddrinfo(servinfo);

	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor) {
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente,&tam_direccion);



	return socket_cliente;
}

int recibir_operacion(int socket_cliente) {
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(uint32_t), MSG_WAITALL) != 0)
		return cod_op;
	else {
		close(socket_cliente);
		return -1;
	}
}


void recibir_buffer(int sockfd, t_buffer* buffer)
{
  //Devuelve el buffer con los datos recibidos. 
	t_buffer temporal;
	int bytes_recibidos;

	bytes_recibidos = recv(sockfd, &temporal.size, sizeof(uint32_t), MSG_WAITALL);
	if(bytes_recibidos > 0)
	{
		temporal.stream = malloc(temporal.size);
		
		bytes_recibidos = recv(sockfd, temporal.stream, temporal.size, MSG_WAITALL);
	
		if(bytes_recibidos > 0)
		{
			buffer_pack(buffer, temporal.stream, temporal.size);
			
		}
		free(temporal.stream);
	}

}

void buffer_pack(t_buffer* buffer, void* stream_to_add, int size) {
	buffer->stream = realloc(buffer->stream, buffer->size + size);
	memcpy(buffer->stream + buffer->size, stream_to_add, size);
	buffer->size += size;
}

void buffer_unpack(t_buffer* buffer, void* dest, int size) {
	memcpy(dest, buffer->stream, size);
	buffer->size -= size;
	memmove(buffer->stream, buffer->stream + size, buffer->size);
	buffer->stream = realloc(buffer->stream, buffer->size);
}
