#include "memoria.h"


//Mock de memoria para probar envios de mensajes con el kernel.

int main(){
	iniciar_logger();    
	inicializar();

	//Hilo para escuchar conexiones del kernel:
	
	pthread_create(&hiloEscucha, NULL, escucharConexiones, NULL);
	pthread_join(hiloEscucha,NULL);


	log_destroy(logger); // TODO finalifza todo, cerrar conexiones, etc.
}

void inicializar(){
	t_config* config = config_create("./cfg/memoria.config");
	char* ip =string_duplicate(config_get_string_value(config, "IP"));
	log_info(logger,"[CONFIGURACION] IP  servidor: %s", ip);
	char* puerto =string_duplicate(config_get_string_value(config, "PUERTO"));
	log_info(logger,"[CONFIGURACION] Puerto  servidor: %s", puerto);
	config_destroy(config);
	server_fd = iniciar_servidor(ip,puerto);
	if(server_fd>0){
		log_info(logger, "Servidor listo para recibir al kernel");
	}else{
		log_warning(logger, "No se pudo crear el servidor.");	
		//TODO ver de finalizar....
	}
}
void iniciar_logger(void)
{
	
	logger=log_create("./cfg/memoria.log","MEMORIA",1,LOG_LEVEL_INFO);

}


void* escucharConexiones() {
	pthread_t threadId[MAX_CONEXIONES];
	int socketDelCliente[MAX_CONEXIONES];
	int contadorConexiones = 0;
	while (1) {
		socketDelCliente[contadorConexiones] = esperar_cliente(server_fd);

		if (socketDelCliente >= 0) {

			log_info(logger, "Se ha aceptado una conexion: %i\n", socketDelCliente[contadorConexiones]);
			if ((pthread_create(&threadId[contadorConexiones], NULL, handler,
					(void*) &socketDelCliente[contadorConexiones])) < 0) {
				
				
			   
			} else {
				log_info(logger, "Handler asignado\n");
				pthread_detach(threadId[contadorConexiones]);
			}
		} else {
			log_info(logger, "Falló al aceptar conexión");
		}

		contadorConexiones++;
	}    
}


void* handler(void* socketConectado) {
	//Aca resolvere lo que tiene que hacer con cada mensaje...
	int socket = *(int*) socketConectado;
	//log_info(logger, "Estoy en el Handler. Soy la conexion %i\n",socket);

	op_code codigo_operacion = recibir_operacion(socket);
	int size;
	t_buffer* buffer=crear_tbuffer();
	recibir_buffer(socket,buffer);

		switch (codigo_operacion) {
		case INIT:
			procesar_init( buffer,  socket);
			break;
		case CLOSE:
			procesar_close(buffer, socket);
			log_info(logger, "Recibi codigo de operacion: CLOSE");
			break;
		case MEMALLOC:
            procesar_memalloc( buffer, socket);
			break;
		case MEMFREE:
			procesar_memfree(buffer, socket);
			break;
		case MEMREAD:
			procesar_memread(buffer, socket);
				log_info(logger, "Recibi codigo de operacion: MEMREAD");
			break;	
		case SEM_INIT:
				log_info(logger, "Recibi codigo de operacion: SEM_INIT - Este codigo no lo maneja memoria");
			break;
		case SEM_WAIT:
				log_info(logger, "Recibi codigo de operacion: SEM_WAIT - Este codigo no lo maneja memoria");
			break;
		case SEM_POST:
				log_info(logger, "Recibi codigo de operacion: SEM_POST - Este codigo no lo maneja memoria ");
			break;
		case SEM_DESTROY:
				log_info(logger, "Recibi codigo de operacion: SEM_DESTROY - Este codigo no lo maneja memoria");
			break;			
		case CALL_IO:
				log_info(logger, "Recibi codigo de operacion: CALL_IO - Este codigo no lo maneja memoria");
			break;		
		case MEMWRITE:
			procesar_memwrite(buffer, socket);
				log_info(logger, "Recibi codigo de operacion: MEMWRITE");
			break;	
		case SUSPENDER:
				log_info(logger, "Recibi codigo de operacion: SUSPENDER");
				procesar_suspender(buffer, socket);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return (void*) EXIT_FAILURE;
		default:
			log_warning(logger,
				"Operacion desconocida.");
			break;
		}

}


e_remitente  buffer_unpack_init( buffer){
	e_remitente remitente;
	buffer_unpack( buffer, &(remitente), sizeof(uint32_t));
	return remitente;
}

int buffer_unpack_memalloc(t_buffer *buffer){

	
    int pid;
	int tamanio;
    buffer_unpack(buffer, &pid,sizeof(uint32_t) );
	buffer_unpack(buffer, &tamanio,sizeof(uint32_t) );

	log_info(logger, "MEMALLOC Procesada  Pid: %i Tamanio:  %i",pid,  tamanio);
	return STATUS_SUCCESS;
}
void  procesar_close(t_buffer* buffer, int socket){
 	int pid;
	
    buffer_unpack(buffer, &pid,sizeof(uint32_t) );
	log_info(logger, "CLOSE Procesado  Pid: %i ",pid);
	enviar_status(STATUS_SUCCESS, socket);


}

void procesar_init(void* buffer, int socket){
    log_info(logger, "Recibi codigo de operacion: INIT \n");
	e_remitente remitente = buffer_unpack_init( buffer);
	log_info(logger,"Recibi el remitente %i", remitente);
	int pid=0;
	if(remitente==MATELIB){
		log_info(logger, "EL remitente es matelib");
		//REMITENTE = MATELIB --> Asigno PID.
		pid+=1;
		//Tengo que comunicarme con matelib y enviarle el pid que genere.
	}else if(remitente==KERNEL){
		//t_buffer *buffer = crear_tbuffer();
		//recv(socket,&pid, sizeof(uint32_t), MSG_WAITALL );
		//recibir_buffer(socket, buffer);
		buffer_unpack(buffer,&pid,sizeof(uint32_t));
		log_info(logger, "Desempaquete el pid %i", pid);
	}
	e_status status = STATUS_SUCCESS;
	

    //Mandar status 
	enviar_status(status, socket);
    //void* a_enviar= malloc(sizeof(uint32_t));
	//memcpy(a_enviar, &status, sizeof(uint32_t));
	//send(socket, a_enviar, sizeof(uint32_t),0);

	//free(a_enviar);		
    log_info(logger, "Ya mande el status: %s \n",status_to_string(status));
	liberar_conexion(socket);
}
void procesar_memalloc(void* buffer, int socket)
{
    log_info(logger, "Recibi codigo de operacion: MEMALLOC \n");
    buffer_unpack_memalloc(buffer);

	//Simulo que envio la direccion
	void* a_enviar=malloc(sizeof(uint32_t));
	int address= 1000;
	memcpy(a_enviar, &address, sizeof(uint32_t));
	send(socket, a_enviar,sizeof(uint32_t),0);

    liberar_conexion(socket);
}

void procesar_suspender(t_buffer* buffer, int socket){
	int pid;
    log_info(logger, "Recibi codigo de operacion: SUSPENDER \n");
	buffer_unpack(buffer,&pid, sizeof(uint32_t) );
	log_info(logger, "Recibi SUSPENDER Carpincho id:%i", pid);
	enviar_status(STATUS_SUCCESS,socket);
    liberar_conexion(socket);
}

void enviar_buffer(int socket,  t_buffer* buffer)
{
	int bytes = buffer->size +  sizeof(uint32_t); // Mando todo lo del buffer  + el tamaño de bytes
	void* a_enviar = malloc(bytes);
	int offset = 0;


	memcpy(a_enviar + offset, &(buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, buffer->stream, buffer->size);

	free(buffer->stream);
	free(buffer);

	int bytes_enviados= send(socket , a_enviar , bytes,0);
	if(bytes_enviados!=bytes){
		log_warning(logger, "Revisar envio de buffer. Bytes enviados: %i", bytes_enviados);
	}
	free(a_enviar);
}


void procesar_memread(t_buffer* buffer, int socket)
{
	int pid;
	int origin;
	int size;

    log_info(logger, "Recibi codigo de operacion: MEMFREE \n");
	buffer_unpack(buffer,&pid, sizeof(uint32_t) );
    buffer_unpack(buffer,&origin, sizeof(uint32_t) );
	buffer_unpack(buffer,&size, sizeof(uint32_t) );
	log_info(logger, "Recibi MEMREAD  Carpincho id:%i  Origin: %i Size %i ", pid, origin, size);
	
	
	//Respuesta:
	t_buffer* buffer_rta= crear_tbuffer();
	int status = STATUS_SUCCESS;
	char* read= "HOLA";
	buffer_pack(buffer_rta, &status, sizeof(uint32_t) );
	buffer_pack(buffer_rta, read, size);
	
	enviar_buffer(socket, buffer_rta);

    liberar_conexion(socket);
	
}
char* buffer_unpack_string(t_buffer* buffer) 
{
	char* dest;
	uint32_t length;

	buffer_unpack(buffer, &length, sizeof(uint32_t));
	dest = calloc(length + 1, sizeof(char));
	buffer_unpack(buffer, dest, length);

	return dest;
}

void procesar_memwrite(t_buffer* buffer, int socket)
{
	int pid;
	int destino;
	char* string;

    log_info(logger, "Recibi codigo de operacion: MEMFREE \n");
	buffer_unpack(buffer,&pid, sizeof(uint32_t) );
    buffer_unpack(buffer,&destino, sizeof(uint32_t) );
	string= buffer_unpack_string(buffer);
	log_info(logger, "Recibi MEMWRITE  Carpincho id:%i  Destino: %i WRITE: %s ", pid, destino, string);
	
	
	//Respuesta:
	int status = STATUS_SUCCESS;
	enviar_status(status, socket);

    liberar_conexion(socket);
	
}

void buffer_pack_string(t_buffer* buffer, char* string_to_add) {
	uint32_t length = strlen(string_to_add);
	buffer_pack(buffer, &length, sizeof(uint32_t));

	buffer->stream = realloc(buffer->stream, buffer->size + length);
	memcpy(buffer->stream + buffer->size, string_to_add, length);
	buffer->size += length;
}


void procesar_memfree(t_buffer* buffer, int socket)
{
	int pid;
	int address;
    log_info(logger, "Recibi codigo de operacion: MEMFREE \n");
	buffer_unpack(buffer,&pid, sizeof(uint32_t) );
    buffer_unpack(buffer,&address, sizeof(uint32_t) );
	log_info(logger, "Recibi MEMFREE Carpincho id:%i  Address: %i ", pid, address);
	enviar_status(STATUS_SUCCESS,socket);
    liberar_conexion(socket);
}
void enviar_status(e_status status, int socket ){
	void* a_enviar=malloc(sizeof(uint32_t));
	memcpy(a_enviar, &status, sizeof(uint32_t));
	send(socket, a_enviar,sizeof(uint32_t),0);
	log_info(logger, "%s enviado", status_to_string (status));
	free(a_enviar);
}

char* status[] =
{
	"STATUS_ERROR",
    "STATUS_SUCCESS"
};

const char* status_to_string(int valor)
{
	return status[valor+1];
}
e_status recibir_status( int socket ){
	e_status status;
	void* recibido=malloc(sizeof(uint32_t));

	recv(socket, recibido,sizeof(uint32_t),MSG_WAITALL);

	memcpy(&status, recibido, sizeof(uint32_t));
	log_info(logger, "%s recibido", status_to_string (status));
	free(recibido);
  return status;
}