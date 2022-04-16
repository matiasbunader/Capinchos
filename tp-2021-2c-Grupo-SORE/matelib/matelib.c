#include<sys/socket.h>
#include<netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/string.h>
#include "matelib.h"


 char* status[] =
{
  "MATE_ALLOC_FAULT",//-8
	"MATE_WRITE_FAULT",//-7
	"MATE_READ_FAULT", //-6
	"MATE_FREE_FAULT", //-5
	"STATUS_SEM_INEXISTENTE", //-4
	"DEVICE_NOT_FOUND",//-3
	"MATE_CONNECTION_FAULT",//-2
	"STATUS_ERROR", //-1
  "STATUS_SUCCESS",//0
};

//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *path_config)
{   
  inicializar(lib_ref, path_config);
  int status = solicitar_init(lib_ref);

  return status; 
}


int mate_close(mate_instance *lib_ref)
{
 // printf("Estoy por enviar un mensaje  mate_close del pid %i \n",lib_ref->group_info->pid);

	t_buffer *buffer = crear_tbuffer();
  buffer_pack(buffer,&(lib_ref->group_info->pid) ,sizeof(uint32_t));
  enviar_mensaje(CLOSE, buffer, lib_ref->group_info->conexion);
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_CLOSE - Solicitud enviada. \n",lib_ref->group_info->pid);
  int status= recibir_status(lib_ref->group_info->conexion);
  if(status != 0){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_CLOSE - Status recibido[%i]: %s",lib_ref->group_info->pid,status, status_to_string(status) );


  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_CLOSE - Status recibido[%i]: %s",lib_ref->group_info->pid,status, status_to_string(status) );
  }
  
  close(lib_ref->group_info->conexion);
  free(lib_ref->group_info);

  return 0;
}


//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {
  

  t_buffer *buffer =  crear_tbuffer();
  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack_string(buffer, sem);
  buffer_pack( buffer, &(value), sizeof(uint32_t));
  enviar_mensaje(SEM_INIT, buffer, lib_ref->group_info->conexion);  
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_INIT - Solicitud enviada",lib_ref->group_info->pid);
  int status= recibir_status(lib_ref->group_info->conexion);
  if(status !=0 ){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_INIT - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_INIT - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }

  return status;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {
  
  //Serializar y enviar SEM_WAIT
  t_buffer *buffer =  crear_tbuffer();
  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack_string(buffer, sem);
  enviar_mensaje(SEM_WAIT, buffer, lib_ref->group_info->conexion);  
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_WAIT - Solicitud enviada",lib_ref->group_info->pid);
  //e_status status= recibir_status(lib_ref->group_info->conexion);
  
  t_buffer *buffer_rta =  crear_tbuffer();
  recibir_buffer(lib_ref->group_info->conexion, buffer_rta);
  int status;

  if(buffer_rta->size >0){
    buffer_unpack(buffer_rta, &status,sizeof(uint32_t));
  }else{
    status= MATE_CONNECTION_FAULT;
  }
  
 
  if(status !=0){
    //error
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_WAIT - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_WAIT - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }
 
  liberar_buffer(buffer_rta);
  return status;
 
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) {

  t_buffer *buffer =  crear_tbuffer();
  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack_string(buffer, sem);
  
  enviar_mensaje(SEM_POST, buffer, lib_ref->group_info->conexion);  
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_POST - Solicitud enviada",lib_ref->group_info->pid);

  t_buffer *buffer_rta =  crear_tbuffer();
  recibir_buffer(lib_ref->group_info->conexion, buffer_rta);
  int status;
  
  
  if(buffer_rta->size >0){
    buffer_unpack(buffer_rta, &status,sizeof(uint32_t));
  }else{
    status= MATE_CONNECTION_FAULT;
  }

  if(status !=0){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_POST - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }else{
     log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_POST - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }
 
  liberar_buffer(buffer_rta);
  return status;
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {

  t_buffer *buffer =  crear_tbuffer();
  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack_string(buffer, sem);
  enviar_mensaje(SEM_DESTROY, buffer, lib_ref->group_info->conexion);  

  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_DESTROY - %s Solicitud enviada",lib_ref->group_info->pid, sem);
  int status= recibir_status(lib_ref->group_info->conexion);
  if(status !=0){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_DESTROY - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }else{
   log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_SEM_DESTROY - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }
  
  return status;


}


//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{

  t_buffer *buffer =  crear_tbuffer();
  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack_string(buffer, io);
  //todo ver para que es el *msg
  enviar_mensaje(CALL_IO, buffer, lib_ref->group_info->conexion);  
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_CALL_IO - Solicitud enviada", lib_ref->group_info->pid);
  
  t_buffer *buffer_rta =  crear_tbuffer();
  recibir_buffer(lib_ref->group_info->conexion, buffer_rta);
  int status;

  if(buffer_rta->size >0){
    buffer_unpack(buffer_rta, &status,sizeof(uint32_t));
  }else{
    status= MATE_CONNECTION_FAULT;
  }
  
  if(status!=0){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_CALL_IO - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_CALL_IO - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }
  
  liberar_buffer(buffer_rta);
  return status;

}

//--------------Memory Module Functions-------------------/


//Devuelve -1 si la operacion no se pudo realizar. 
mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{
  t_buffer *buffer = crear_tbuffer();

  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack( buffer, &(size), sizeof(uint32_t));
  enviar_mensaje(MEMALLOC, buffer, lib_ref->group_info->conexion); 

  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMALLOC - Solicitud enviada", lib_ref->group_info->pid);
  mate_pointer address;
  int recibido;

  int bytes_recibidos= recv(lib_ref->group_info->conexion, &recibido, sizeof(uint32_t), MSG_WAITALL);

  if(bytes_recibidos >0){
    memcpy(&address, &recibido, sizeof(int32_t));
    if(address == -1){
     int status = -8; 
     //Todo: AGREGO UN CODIGO DE ERROR?
      log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMALLOC - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(recibido) );
    }else{
      //log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEM_ALLOC - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
      log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMALLOC - Procesado", lib_ref->group_info->pid );
    }
  }else{
    int status= MATE_CONNECTION_FAULT;
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMALLOC - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }
  return address;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{
  t_buffer *buffer = crear_tbuffer();

  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack( buffer, &(addr), sizeof(uint32_t));
  enviar_mensaje(MEMFREE, buffer, lib_ref->group_info->conexion); 
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMFREE - Solicitud enviada", lib_ref->group_info->pid);
  
  int status= recibir_status(lib_ref->group_info->conexion);

  if(status !=0){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMFREE - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMFREE - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }
  
  return status;
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
  t_buffer *buffer = crear_tbuffer();

  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack( buffer, &(origin), sizeof(uint32_t));
  buffer_pack( buffer, &(size), sizeof(uint32_t));
  enviar_mensaje(MEMREAD, buffer, lib_ref->group_info->conexion); 
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMREAD - Solicitud enviada", lib_ref->group_info->pid);

  t_buffer *buffer_rta =  crear_tbuffer();
  recibir_buffer(lib_ref->group_info->conexion, buffer_rta);
  int status;
  
  if(buffer_rta->size >0){
    buffer_unpack(buffer, &status, sizeof(uint32_t));
    if(status ==0){
      log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMREAD - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
      buffer_unpack(buffer, dest, size);
    }else{
       log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMREAD - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
    }  
  }else{
    status= MATE_CONNECTION_FAULT;
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMREAD - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
 
  }
  
  return status;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
  t_buffer *buffer = crear_tbuffer();

  buffer_pack( buffer, &(lib_ref->group_info->pid), sizeof(uint32_t));
  buffer_pack( buffer, &(dest), sizeof(uint32_t));
  buffer_pack( buffer, &(size), sizeof(uint32_t));
  buffer_pack( buffer, origin, size);
  enviar_mensaje(MEMWRITE, buffer, lib_ref->group_info->conexion); 
  log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMWRITE - Solicitud enviada", lib_ref->group_info->pid);

  int status= recibir_status(lib_ref->group_info->conexion);

  if(status !=0){
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMWRITE - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_MEMWRITE - Status recibido[%i]: %s",lib_ref->group_info->pid, status, status_to_string(status) );
  }
  return status;
}

//------------------Otras---------------------/

 void buffer_pack_string(t_buffer* buffer, char* string_to_add) {
	uint32_t length = strlen(string_to_add);
	buffer_pack(buffer, &length, sizeof(uint32_t));

	buffer->stream = realloc(buffer->stream, buffer->size + length);
	memcpy(buffer->stream + buffer->size, string_to_add, length);
	buffer->size += length;
}



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


 void enviar_mensaje(op_code codigo_operacion, t_buffer* buffer, int socket){
	//printf("Size original: %i\n", buffer->size);
	int bytes_a_enviar = buffer->size + 2* sizeof(uint32_t);

	void* a_enviar = malloc(bytes_a_enviar);
  //printf("Hice malloc de %i bytes\n", bytes_a_enviar);
  //printf("Codigo de operacion %i\n", codigo_operacion);

	int offset = 0;
	memcpy(a_enviar + offset, &(codigo_operacion), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, &(buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset,buffer->stream, buffer->size);
  
  send(socket, a_enviar, bytes_a_enviar, MSG_NOSIGNAL);
  //printf("Resultado del envio al socket %i : %i bytes", socket, resultado_envio);
  liberar_buffer(buffer);
  free(a_enviar);


}


 void buffer_pack(t_buffer* buffer, void* stream_to_add, int size) {
	buffer->stream = realloc(buffer->stream, buffer->size + size);
	memcpy(buffer->stream + buffer->size, stream_to_add, size);
	buffer->size += size;
}


 t_buffer* crear_tbuffer() {
	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->size = 0;
	buffer->stream = NULL;

	return buffer;
}


 char* status_to_string(int valor)
{
	return status[valor+8];
}


  t_log* iniciar_logger(void)
{
	t_log* logger=log_create("matelib.log","MATELIB",1,LOG_LEVEL_DEBUG);
  return logger;
    
}

 int recibir_status( int socket ){
  int status;
	void* recibido=malloc(sizeof(uint32_t));
  int bytes_recibidos;
	bytes_recibidos= recv(socket, recibido,sizeof(uint32_t),MSG_WAITALL);

  if(bytes_recibidos >0){
    memcpy(&status, recibido, sizeof(uint32_t));
    
  }else{
    status= MATE_CONNECTION_FAULT;  
  }

	free(recibido);

  return status;
}


 void liberar_buffer(t_buffer* buffer){
  if(buffer->stream !=NULL){
    free(buffer->stream);
  }
  free(buffer);
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
	}else{
     // printf("Error al recibir el buffer, Recibido: %i", bytes_recibidos);
    
  }

}

//Funciones de deserializacion:
 void buffer_unpack(t_buffer* buffer, void* dest, int size) {
	memcpy(dest, buffer->stream, size);
	buffer->size -= size;
	memmove(buffer->stream, buffer->stream + size, buffer->size);
	buffer->stream = realloc(buffer->stream, buffer->size);
}

 t_msj_init* buffer_unpack_init(t_buffer* buffer){
	
	t_msj_init *msj= malloc(sizeof(t_msj_init));
	buffer_unpack( buffer, &(msj->pid), sizeof(uint32_t));
	buffer_unpack( buffer, &(msj->status), sizeof(uint32_t));

	return msj;

}


 void inicializar(mate_instance *lib_ref, char *path_config)
{
  //Inicializa la matelib con sus Estructuras internas. Crea la conexion con memoria que luego se va a mantener viva. 
  lib_ref->group_info = malloc(sizeof(mate_inner_structure));
  //t_config* config= config_create("../cfg/matelib.config");
  t_config* config= config_create(path_config);
  lib_ref->group_info->logger=iniciar_logger();
  lib_ref->group_info->ip=string_duplicate(config_get_string_value(config, "IP"));
  lib_ref->group_info->puerto=string_duplicate(config_get_string_value(config, "PUERTO"));

  //TODO Considerar que puede que no se logre conectar.
  lib_ref->group_info->conexion = crear_conexion(lib_ref->group_info->ip,lib_ref->group_info->puerto);
  config_destroy(config);
}


 int  solicitar_init(mate_instance *lib_ref)
{

  //Mando un mensaje con el codigo de operacion INIT.
  t_buffer* buffer=crear_tbuffer();
  e_remitente remitente= MATELIB;
  buffer_pack(buffer,&remitente,sizeof(uint32_t));

  enviar_mensaje(INIT, buffer, lib_ref->group_info->conexion);
  log_info(lib_ref->group_info->logger, "[CARPINCHO] Mensaje: MATE_INIT");

  //Recibo la respuesta si se pudo inicializar correctamente.(PID + STATUS)
  
  
  t_buffer* buffer_rec=crear_tbuffer();
  int status;
  recibir_buffer(lib_ref->group_info->conexion, buffer_rec);

  if(buffer_rec->size >0){
    t_msj_init *msj= buffer_unpack_init(buffer_rec);
    //Guardo el pid recibido
    lib_ref->group_info->pid = msj->pid;
    status=msj->status;
     free(msj);
  }else{
    status= MATE_CONNECTION_FAULT;
  }
  

  if(status!= 0){
    //ERROR
    log_error(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_INIT Procesado - Status[%i]: %s", lib_ref->group_info->pid,status, status_to_string(status));
  }else{
    log_info(lib_ref->group_info->logger, "[CARPINCHO %i] MATE_INIT Procesado - Status[%i]: %s",lib_ref->group_info->pid, status,  status_to_string(status));
  }

  liberar_buffer(buffer_rec);
 
  return status;

}

