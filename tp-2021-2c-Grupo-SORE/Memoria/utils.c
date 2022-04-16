#include "utils.h"

void *iniciar_servidor()
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IPKernel, puertoKernel, &hints, &servinfo);

	if ((socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype,
								  servinfo->ai_protocol)) == -1)
	{
		printf("\nNo se creo correctamente el servidor\n");
	}
	else
	{
		printf("\nSe creo correctamente el servidor\n");
	}

	if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		close(socket_servidor);
		printf("No se creo correctaamente el servidor");
	}
	else
	{
		printf("\nSe realizo el bind correctamente\n");
	}

	if (listen(socket_servidor, SOMAXCONN))
	{
		printf("\nNo se creo correctamente el servidor\n");
	}
	else
	{
		printf("\nEl servidor esta listo para escuchar\n");
	}

	freeaddrinfo(servinfo);

	while (1)
	{
		esperar_cliente(socket_servidor);
	}
}
void esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	pthread_t hilo;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	socket_cliente = accept(socket_servidor, (void *)&dir_cliente,
							&tam_direccion);

	log_info(logger, "\nSe conecto un cliente\n");

	int *socketAPasar = malloc(sizeof(int));
	*socketAPasar = socket_cliente;

	pthread_create(&hilo, NULL, (void *)recibir_operacion, socketAPasar);
	pthread_detach(hilo);
}

void recibir_operacion(int *socket_cliente)
{
	int socket = *socket_cliente;

	free(socket_cliente);
	Cod_op cod_op;
	recv(socket, &cod_op, sizeof(Cod_op), MSG_WAITALL);
	if (cod_op != SALIR)
	{

		procesar_mensaje(cod_op, socket);
	}
	else
	{
		printf("Salir y cerrar conexion");
		close(socket);
		pthread_exit(NULL);
	}
}

void procesar_mensaje(uint32_t codop, int socket)
{
	void *buffer = recibir_buffer(socket);
	uint32_t pid;

	switch (codop)
	{
	case MATE_INIT:

		IniciarProcesoMemoria(buffer, socket);
		break;
	case MATE_MEMALLOC:
		realizarMemalloc(buffer, socket);
		break;
	case MATE_MEMFREE:
		realizarMemFree(buffer, socket);
		
		break;
	case MATE_MEMREAD:
		RealizarLecturaPaginas(buffer, socket);

		break;
	case MATE_MEMWRITE:

		realizarEscrituraMemoria(buffer, socket);
		break;
	case MATE_SUSPENDER:
		memcpy(&pid, buffer, sizeof(uint32_t));
		log_info(logger, "numero PID recibido %d", pid);
		realizarSuspencionProceso(pid, socket);
		break;
	case CLOSE:
		memcpy(&pid, buffer, sizeof(uint32_t));
		log_info(logger, "numero PID recibido %d", pid);
		liberarCarpinchoMemoria(pid);
		uint32_t status = 0;
		send(socket, &status, sizeof(uint32_t), 0);
		log_info(logger, "Se envio el status con exito ");
		break;


	default:
		break;
	}
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}
void liberarBuffer(t_buffer *buffer)
{
	if ((buffer->stream) != NULL)
	{
		free(buffer->stream);
	}
	if (buffer != NULL)
	{
		free(buffer);
	}
}
Memalloc *deserializarMemalloc(void *buffer)
{
	Memalloc *nuevo = malloc(sizeof(Memalloc));
	int desplazamiento = 0;
	memcpy(&(nuevo->pid), buffer, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(nuevo->size), buffer + desplazamiento, sizeof(uint32_t));
	return nuevo;
}
Memread *deserializarMemread(void *buffer)
{
	Memread *nuevo = malloc(sizeof(Memread));
	int desplazamiento = 0;

	memcpy(&(nuevo->pid), buffer, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo->dirLog), buffer + desplazamiento, sizeof(uint32_t));

	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo->size), buffer + desplazamiento, sizeof(uint32_t));
	return nuevo;
}
Memwrite *deserializarMemwrite(void *buffer)
{
	Memwrite *nuevo = malloc(sizeof(Memwrite));
	int desplazamiento = 0;

	memcpy(&(nuevo->pid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo->dirLog), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo->tamanioCont), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	nuevo->contenido = malloc(nuevo->tamanioCont);
	memcpy(nuevo->contenido, buffer + desplazamiento, nuevo->tamanioCont);

	return nuevo;
}

MemFree *deserializarMemFree(void *buffer)
{
	MemFree *nuevo = malloc(sizeof(MemFree));
	int desplazamiento = 0;

	memcpy(&(nuevo->pid), buffer, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo->dirLog), buffer + desplazamiento, sizeof(uint32_t));
	return nuevo;
}

void *recibir_buffer(int socket_cliente)
{
	void *buffer;
	int size;

	recv(socket_cliente, &(size), sizeof(uint32_t), MSG_WAITALL);
	log_info(logger, "Tamanio del buffer recibido %d", size);
	buffer = malloc(size);
	recv(socket_cliente, buffer, size, MSG_WAITALL);

	return buffer;
}

int crear_conexion()
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IPswamp, puertoSwamp, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family,
								server_info->ai_socktype, server_info->ai_protocol);

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
}
void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}
void *armado_a_enviar(uint8_t codigo_operacion, t_buffer *buffer)
{

	int bytes = buffer->size + sizeof(uint8_t) //Codigo de Operacion
				+ sizeof(uint32_t);			   //Bytes total a enviar.

	void *a_enviar = malloc(bytes);
	int offset = 0;
	memcpy(a_enviar + offset, &codigo_operacion, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(a_enviar + offset, &(buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, buffer->stream, buffer->size);

	liberarBuffer(buffer);
	return a_enviar;
}

void enviarTipoAsignacion(char *tipoAsignacion)
{
	//int conexion = crear_conexion(IPswamp, puertoSwamp);
	int sizeSerializado = 0;

	uint8_t tipo = 0;
	if (string_equals_ignore_case("FIJA", tipoAsignacion))
	{
		tipo = 1;
	}
	else
	{
		tipo = 2;
	}
	log_info(logger, "Enciando tipo %d", tipo);
	void *a_enviar = serializarTipoAsignacion(tipo, &sizeSerializado);
	log_info(logger, "Serializado con exito");
	send(conexionSwamp, a_enviar, sizeSerializado, 0);
	free(a_enviar);
	recv(conexionSwamp, &marcosMaximosSwamp, sizeof(uint32_t), 0);
	log_info(logger, "Los Marcos maximos que puedo tener en Swamp son %d", marcosMaximosSwamp);
}
void *serializarContenidoRespuesta(void *contenido, uint32_t size, uint32_t respuesta, uint32_t *bytes)
{
	int sizeSerializado = 2 * sizeof(uint32_t) + size;
	log_error(logger, "Size serialziado memread %i", sizeSerializado);

	log_info(logger, "Serializado tamanio %d", sizeSerializado);
	uint32_t desplazamientoBuffer = 0;
	void *buffer = malloc(sizeSerializado);
	int size_buffer = sizeof(uint32_t) + size;

	memcpy(buffer + desplazamientoBuffer, &size_buffer, sizeof(uint32_t));
	desplazamientoBuffer += sizeof(uint32_t);

	memcpy(buffer + desplazamientoBuffer, &(respuesta), sizeof(uint32_t));
	desplazamientoBuffer += sizeof(uint32_t);

	memcpy(buffer + desplazamientoBuffer, contenido, size);
	desplazamientoBuffer += size;

	(*bytes) = desplazamientoBuffer;

	return buffer;
}
void *serializarTipoAsignacion(uint8_t tipo, int *bytes)
{
	int sizeSerializado = 2 * sizeof(uint8_t);
	uint8_t codOP = ASIGNACION;

	log_info(logger, "Serializado tamanio %d", sizeSerializado);
	uint32_t desplazamientoBuffer = 0;
	void *a_enviar = malloc(2 * sizeof(uint8_t));

	memcpy(a_enviar + desplazamientoBuffer, &(codOP), sizeof(uint8_t));
	desplazamientoBuffer += sizeof(uint8_t);

	memcpy(a_enviar + desplazamientoBuffer, &(tipo), sizeof(uint8_t));
	desplazamientoBuffer += sizeof(uint8_t);
	(*bytes) = sizeSerializado;

	/*
		int sizeSerializado = sizeof(uint8_t);
	t_buffer* buffer = malloc(sizeof(t_buffer));
		buffer->size = sizeSerializado;
		buffer->stream = malloc(buffer->size);
		int desplazamiento = 0;

		memcpy(buffer->stream + desplazamiento, &(tipo), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);


		(*bytes) = sizeSerializado + sizeof(uint8_t) + sizeof(uint32_t);
		log_info(logger, "Tamanio del buffer antes de armar %d", sizeSerializado);

		void* a_enviar = armado_a_enviar(ASIGNACION, buffer);
*/
	return a_enviar;
}

void *serializarPageFault(uint32_t pidVic, uint16_t paginaVict, void *marco, int *bytes)
{
	int sizeSerializado = (2 * sizeof(uint32_t)) + sizeof(uint16_t) + sizeof(uint8_t) +
						  +tamanioPagina;

	int desplazamiento = 0;
	void *a_enviar = malloc(sizeSerializado);
	uint8_t codOP = ESCRITURA;
	memcpy(a_enviar + desplazamiento, &(codOP), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(a_enviar + desplazamiento, &(pidVic), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(a_enviar + desplazamiento, &(paginaVict), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
	memcpy(a_enviar + desplazamiento, &(tamanioPagina), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(a_enviar + desplazamiento, marco, tamanioPagina);
	(*bytes) = sizeSerializado;

	return a_enviar;
}
void *serializarSolicitudPaginaPageFault(uint16_t pagina, uint32_t pidSo,
										 int *bytes)
{
	int sizeSerializado = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t);
	void *a_enviar = malloc(sizeSerializado);
	int desplazamiento = 0;
	uint8_t codOP = LECTURA;
	memcpy(a_enviar + desplazamiento, &(codOP), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(a_enviar + desplazamiento, &(pidSo), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(a_enviar + desplazamiento, &(pagina), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	(*bytes) = sizeSerializado;
	/*
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeSerializado;
	buffer->stream = malloc(buffer->size);
	int desplazamiento = 0;

	memcpy(buffer->stream + desplazamiento, &(pidSo), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer->stream + desplazamiento, &(pagina), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	(*bytes) = sizeSerializado + sizeof(uint8_t) + sizeof(uint32_t);

	void* a_enviar = armado_a_enviar(LECTURA, buffer);
*/
	return a_enviar;
}
void *serializarConsultaReserva(uint32_t id, uint16_t pagina, uint32_t reserva,
								int *bytes)
{
	int sizeSerializado = (2 * sizeof(uint32_t)) + sizeof(uint16_t) + sizeof(uint8_t);

	int desplazamiento = 0;
	void *a_enviar = malloc(sizeSerializado);
	uint8_t codOP = CONSULTARESERVA;
	memcpy(a_enviar + desplazamiento, &(codOP), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(a_enviar + desplazamiento, &(id), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(a_enviar + desplazamiento, &(pagina), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
	memcpy(a_enviar + desplazamiento, &(reserva), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	(*bytes) = sizeSerializado;
	/*
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeSerializado;
	buffer->stream = malloc(buffer->size);
	int desplazamiento = 0;

	memcpy(buffer->stream + desplazamiento, &(id), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer->stream + desplazamiento, &(pagina), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
	memcpy(buffer->stream + desplazamiento, &(reserva), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	(*bytes) = sizeSerializado + sizeof(uint8_t) + sizeof(uint32_t);

	void* a_enviar = armado_a_enviar(CONSULTARESERVA, buffer);
*/
	return a_enviar;
}
uint32_t enviarConsultaReserva(uint32_t id, uint16_t pagina, uint32_t reserva)
{
	//int conexion = crear_conexion();

	int sizeSerializado = 0;

	void *a_enviar = serializarConsultaReserva(id, pagina, reserva, &sizeSerializado);
	log_info(logger, "Serializado tamanio %d", sizeSerializado);

	send(conexionSwamp, a_enviar, sizeSerializado, 0);
	uint32_t respuesta;
	free(a_enviar);
	recv(conexionSwamp, &respuesta, sizeof(uint32_t), MSG_WAITALL);
	log_info(logger, "Se recibio la respuesta");
	return respuesta;
}

void enviarVictimaPageFault(uint32_t pidVictima, uint16_t paginaVictima, void *marco)
{
	//int conexion = crear_conexion(IPswamp, puertoSwamp);
	int sizeSerializado = 0;
	log_info(logger, "Se Murio?");

	void *a_enviar = serializarPageFault(pidVictima, paginaVictima, marco, &sizeSerializado);
	log_info(logger, "Serializado con exito");
	send(conexionSwamp, a_enviar, sizeSerializado, 0);

	free(a_enviar);

	//	liberar_conexion(conexion);
}
void *solicitarPaginaPageFault(uint16_t paginaSolicitante,
							   uint32_t pidSolicitada)
{
	//int conexion = crear_conexion(IPswamp, puertoSwamp);
	int sizeSerializado = 0;
	
	void *a_enviar = serializarSolicitudPaginaPageFault(paginaSolicitante,
														pidSolicitada, &sizeSerializado);

	log_info(logger, "Serializado con exito");
	send(conexionSwamp, a_enviar, sizeSerializado, 0);

	free(a_enviar);

	void *marco = malloc(tamanioPagina);



	recv(conexionSwamp, marco, tamanioPagina, 0);


	

	return marco;
}
void realizarEnvioLiberacioMarco(uint32_t pid,uint16_t pagina){
	log_warning(logger,"Se quiere liberar marco en swamp de Pagina %d y PID: %d"
	,pagina,pid);
	
	uint32_t desplazamiento=0;
	uint8_t codigo=LIBERARMARCO;
	void *buffer=malloc(sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint8_t));
	memcpy(buffer,&codigo,sizeof(uint8_t));
	desplazamiento+=sizeof(uint8_t);
	memcpy(buffer+desplazamiento,&pid,sizeof(uint32_t));
    desplazamiento+=sizeof(uint32_t);
    memcpy(buffer+desplazamiento,&pagina,sizeof(uint16_t));
	desplazamiento+=sizeof(uint16_t);
	send(conexionSwamp,buffer,desplazamiento,0);
	log_info(logger,"Envio realizado con exito");
	
}
/*

void* serializarContenidoRespuesta(void*contenido,uint32_t size,uint32_t respuesta, uint32_t*bytes) {
	int sizeSerializado = sizeof(uint32_t) + size;

	log_info(logger, "Serializado tamanio %d", sizeSerializado);
	uint32_t desplazamientoBuffer=0;
	void* buffer = malloc(sizeSerializado);


	memcpy(buffer + desplazamientoBuffer, &(respuesta), sizeof(uint32_t));
	desplazamientoBuffer += sizeof(uint32_t);

	memcpy(buffer + desplazamientoBuffer, contenido, size);

	(*bytes)=sizeSerializado;


	return buffer;
}
void* serializarTipoAsignacion(uint8_t tipo,int*bytes) {
	int sizeSerializado = 2*sizeof(uint32_t);

	uint8_t codOP=ASIGNACION;

	log_info(logger, "Serializado tamanio %d", sizeSerializado);
	uint32_t desplazamientoBuffer=0;
	void*a_enviar=malloc(2*sizeof(uint8_t));

	memcpy(a_enviar + desplazamientoBuffer, &(codOP), sizeof(uint8_t));
	desplazamientoBuffer += sizeof(uint8_t);

	memcpy(a_enviar + desplazamientoBuffer, &(tipo), sizeof(uint8_t));
	desplazamientoBuffer += sizeof(uint8_t);
	(*bytes)=sizeSerializado;


	return a_enviar;
}

void* serializarPageFault( uint32_t pidVic,	uint16_t paginaVict, void* marco,int*bytes){
	int sizeSerializado = (2 * sizeof(uint32_t)) +  sizeof(uint16_t)+
			tamanioPagina+sizeof(uint8_t) ;
	uint32_t desplazamiento=0;
	void*aEnviar=malloc(sizeSerializado);
	uint8_t codigoOp=ESCRITURA;

	memcpy(aEnviar + desplazamiento, &(codigoOp), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);

	memcpy(aEnviar + desplazamiento, &(pidVic), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(aEnviar- + desplazamiento, &(paginaVict), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(aEnviar- + desplazamiento, &(tamanioPagina), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(aEnviar + desplazamiento, marco, tamanioPagina);
    (*bytes)=sizeSerializado;

	log_info(logger, "Tamanio del buffer a enviar %d", sizeSerializado);


	return aEnviar;

}
void* serializarSolicitudPaginaPageFault(uint16_t pagina, uint32_t pidSo,
		int*bytes) {
	int sizeSerializado =  sizeof(uint32_t) +  sizeof(uint16_t)+ sizeof(uint8_t);

	uint32_t desplazamiento=0;
	void*aEnviar=malloc(sizeSerializado);
	uint8_t codigoOp=LECTURA;

	memcpy(aEnviar + desplazamiento, &(codigoOp), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);

	memcpy(aEnviar + desplazamiento, &(pidSo), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(aEnviar- + desplazamiento, &(pagina), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
	(*bytes)=sizeSerializado;


	log_info(logger, "Tamanio del buffer a enviar %d", sizeSerializado);


	return aEnviar;
}
void* serializarConsultaReserva(uint32_t id, uint16_t pagina, uint32_t reserva,
		int*bytes) {
	int sizeSerializado = (2 * sizeof(uint32_t)) + sizeof(uint16_t)+sizeof(uint8_t);
	uint32_t desplazamiento=0;

	void*aEnviar=malloc(sizeSerializado);

	uint8_t codigoOp=CONSULTARESERVA;

	memcpy(aEnviar + desplazamiento, &(codigoOp), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);

	memcpy(aEnviar + desplazamiento, &(id), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(aEnviar + desplazamiento, &(pagina), sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
	memcpy(aEnviar + desplazamiento, &(reserva), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	(*bytes) = sizeSerializado ;



	return aEnviar;
}
uint32_t enviarConsultaReserva(uint32_t id, uint16_t pagina, uint32_t reserva) {
	int conexion = crear_conexion();

	int sizeSerializado = 0;

	void * a_enviar =  serializarConsultaReserva (id, pagina, reserva, &sizeSerializado);
	log_info(logger, "Serializado tamanio %d", sizeSerializado);

	send(conexion, a_enviar, sizeSerializado, 0);

	uint32_t respuesta;

	free(a_enviar);
	recv(conexion, &respuesta, sizeof(uint32_t), MSG_WAITALL);
	liberar_conexion(conexion);
	return respuesta;
}

void enviarVictimaPageFault(uint32_t pidVictima, uint16_t paginaVictima, void* marco) {
	int conexion = crear_conexion(IPswamp, puertoSwamp);
	int sizeSerializado = 0;

	void * a_enviar = serializarPageFault(pidVictima, paginaVictima, marco, &sizeSerializado);
	log_info(logger, "Serializado con exito");
	send(conexion, a_enviar, sizeSerializado, 0);

	free(a_enviar);
	liberar_conexion(conexion);
}

void* solicitarPaginaPageFault(uint16_t paginaSolicitante,
		uint32_t pidSolicitada) {
	int conexion = crear_conexion(IPswamp, puertoSwamp);
	int sizeSerializado = 0;

	void * a_enviar = serializarSolicitudPaginaPageFault(paginaSolicitante,
			pidSolicitada, &sizeSerializado);
	log_info(logger, "Serializado con exito");
	send(conexion, a_enviar, sizeSerializado, 0);
	free(a_enviar);

	void*marco = malloc(tamanioPagina);
	recv(conexion, marco, tamanioPagina, MSG_WAITALL);
	log_info(logger, "Se recibio con exito la pagina");

	liberar_conexion(conexion);
	return marco;
}

void enviarTipoAsignacion(char* tipoAsignacion) {
	int conexion = crear_conexion(IPswamp, puertoSwamp);
	int sizeSerializado = 0;
    uint8_t tipo=0;
	if(string_equals_ignore_case("FIJA", tipoAsignacion)){
		tipo=1;
	}else{
		tipo=2;
	}
	void * a_enviar = serializarTipoAsignacion(tipo,&sizeSerializado);
	log_info(logger, "Serializado con exito");
	send(conexion, a_enviar, sizeSerializado, 0);
	recv(conexion,&marcosMaximosSwamp,sizeof(uint32_t),0);
	log_info(logger,"Los Marcos maximos que puedo tener en Swamp son %d",marcosMaximosSwamp);

	free(a_enviar);


}
*/
