#include "send.h"


//Funciones de serializacion, deserializacion y envío de mensajes.

void enviar_init(uint32_t pid, int socket)
{

	t_buffer *buffer = crear_tbuffer();
	e_remitente remitente= KERNEL;

	buffer_pack(buffer,&remitente, sizeof(uint32_t));
	buffer_pack(buffer,&pid, sizeof(uint32_t));
	enviar_mensaje(INIT, buffer,socket);

}


t_buffer* buffer_pack_init_ml(uint32_t pid, t_status status)
{

	t_buffer *buffer = crear_tbuffer();
	//int size= sizeof(uint32_t)*2;
	//buffer_pack(buffer,&size, sizeof(uint32_t));
	buffer_pack(buffer,&pid, sizeof(uint32_t));
	buffer_pack(buffer,&status, sizeof(uint32_t));

	return buffer;
}


void enviar_close(int pid, int socket)
{

	t_buffer *buffer = crear_tbuffer();
	buffer_pack(buffer,&pid,sizeof(uint32_t) );	
	enviar_mensaje(CLOSE,buffer, socket);
}


void enviar_memalloc(t_msj_memalloc *msj, int socket)
{
	
	t_buffer *buffer = crear_tbuffer();
	buffer_pack(buffer,&(msj->pid),sizeof(uint32_t) );
	buffer_pack(buffer,&(msj->tamanio),sizeof(uint32_t) );
	
	enviar_mensaje(MEMALLOC,buffer, socket);
}


void enviar_memfree(t_msj_memfree *msj, int socket)
{

	t_buffer *buffer = crear_tbuffer();
	buffer_pack(buffer,&(msj->pid),sizeof(uint32_t) );
	buffer_pack(buffer,&(msj->add),sizeof(uint32_t) );
	
	enviar_mensaje(MEMFREE,buffer, socket);
}


void enviar_memread(t_msj_memread *msj, int socket)
{

	t_buffer *buffer = crear_tbuffer();
	buffer_pack(buffer,&(msj->pid),sizeof(uint32_t) );
	buffer_pack(buffer,&(msj->origin),sizeof(uint32_t) );
	buffer_pack(buffer,&(msj->size),sizeof(uint32_t) );
	
	enviar_mensaje(MEMREAD,buffer, socket);
}

void enviar_memwrite(t_msj_memwrite *msj, int socket)
{

	t_buffer *buffer = crear_tbuffer();
	buffer_pack(buffer,&(msj->pid),sizeof(uint32_t) );
	buffer_pack(buffer,&(msj->dest),sizeof(uint32_t) );
	buffer_pack_string(buffer,msj->origin);
	
	enviar_mensaje(MEMWRITE,buffer, socket);
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


void buffer_pack_string(t_buffer* buffer, char* string_to_add) {
	uint32_t length = strlen(string_to_add);
	buffer_pack(buffer, &length, sizeof(uint32_t));

	buffer->stream = realloc(buffer->stream, buffer->size + length);
	memcpy(buffer->stream + buffer->size, string_to_add, length);
	buffer->size += length;
}




//Funciones de deserializacion:

t_msj_init*  buffer_unpack_init( t_buffer* buffer)
{
	t_msj_init* msj= malloc(sizeof(t_msj_init));
	

	buffer_unpack( buffer,&(msj->remitente), sizeof(uint32_t));

	return msj;
}


t_msj_seminit* buffer_unpack_seminit(t_buffer* buffer)
{
	
	t_msj_seminit *msj= malloc(sizeof(t_msj_seminit));
	buffer_unpack( buffer, &(msj->pid), sizeof(uint32_t));
	msj->nombre= buffer_unpack_string(buffer);
	buffer_unpack( buffer, &(msj->value), sizeof(uint32_t));

	return msj;

}


t_msj_semwait* buffer_unpack_semwait(t_buffer* buffer)
{
	
	t_msj_semwait *msj= malloc(sizeof(t_msj_semwait));
	buffer_unpack( buffer, &(msj->pid), sizeof(uint32_t));
	msj->nombre= buffer_unpack_string(buffer);
	//log_debug(logger, "Mensaje SEMWAIT  carpincho id: %i Nombre Semaforo %s ", msj->pid, msj->nombre);
	
	return msj;

}


t_msj_memalloc* buffer_unpack_memalloc(t_buffer* buffer)
{
	t_msj_memalloc *msj=malloc(sizeof(t_msj_memalloc));

	//Recibo los datos del Carpincho
	buffer_unpack( buffer, &(msj->pid), sizeof(uint32_t));
	buffer_unpack( buffer, &(msj->tamanio), sizeof(uint32_t));
	//log_info(logger, "Me pidieron memalloc del carpincho pid %i con size= %i", msj->pid, msj->tamanio);

	return msj;

}


int buffer_unpack_close(t_buffer* buffer)
{
	int pid=0;
	buffer_unpack( buffer, &pid, sizeof(uint32_t));

	return pid;

}


t_msj_memfree* buffer_unpack_memfree(t_buffer* buffer)
{
	t_msj_memfree* msj= malloc(sizeof(t_msj_memfree));
	buffer_unpack(buffer, &msj->pid, sizeof(uint32_t));
	buffer_unpack(buffer, &msj->add, sizeof(uint32_t));

	return msj;
}


t_msj_memread* buffer_unpack_memread(t_buffer* buffer)
{
	t_msj_memread* msj= malloc(sizeof(t_msj_memread));
	buffer_unpack(buffer, &msj->pid, sizeof(uint32_t));
	buffer_unpack(buffer, &msj->origin, sizeof(uint32_t));
	buffer_unpack(buffer, &msj->size, sizeof(uint32_t));

	return msj;
}


t_msj_semdestroy* buffer_unpack_semdestroy(t_buffer* buffer)
{

	t_msj_semdestroy* msj = malloc(sizeof(t_msj_semdestroy));
	buffer_unpack(buffer, &msj->pid, sizeof(uint32_t));
	msj->nombre= buffer_unpack_string(buffer);

	return msj;

}


t_msj_semcallio* buffer_unpack_callio(t_buffer* buffer)
{
	t_msj_semcallio* msj= malloc(sizeof(t_msj_semcallio));
	buffer_unpack(buffer, &msj->pid, sizeof(uint32_t));
	msj->nombre= buffer_unpack_string(buffer);
	
	return msj;
}


t_msj_memwrite* buffer_unpack_memwrite(t_buffer* buffer)
{
	t_msj_memwrite* msj= malloc(sizeof(t_msj_memwrite));
	buffer_unpack(buffer, &msj->pid, sizeof(uint32_t));
	buffer_unpack(buffer, &msj->dest, sizeof(uint32_t));
	msj->origin= buffer_unpack_string(buffer);
	
	return msj;
}


t_msj_sempost*  buffer_unpack_sempost(t_buffer* buffer)
{

	t_msj_sempost* msj= malloc(sizeof(t_msj_sempost));
	buffer_unpack(buffer, &(msj->pid), sizeof(uint32_t));
	msj->nombre= buffer_unpack_string(buffer);

	return msj;
}


void recibir_todo(int sockfd, t_buffer* buffer)
{
	t_buffer temporal;
	int bytes_recibidos;

	bytes_recibidos = recv(sockfd, &temporal.size, sizeof(uint32_t), MSG_WAITALL);
	//log_info(logger, "Recibi %i bytes", bytes_recibidos);
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

char* buffer_unpack_string(t_buffer* buffer) 
{
	char* dest;
	uint32_t length;

	buffer_unpack(buffer, &length, sizeof(uint32_t));
	dest = calloc(length + 1, sizeof(char));
	buffer_unpack(buffer, dest, length);

	return dest;
}


void enviar_status(e_status status, int socket )
{
	void* a_enviar=malloc(sizeof(uint32_t));
	memcpy(a_enviar, &status, sizeof(uint32_t));
	send(socket, a_enviar,sizeof(uint32_t),0);
	free(a_enviar);
}


void enviar_mensaje(op_code codigo_operacion, t_buffer* buffer, int socket)
{
	
	int bytes_a_enviar = buffer->size + 2* sizeof(uint32_t);
	void* a_enviar= armado_a_enviar( codigo_operacion,  buffer);
  	send(socket,a_enviar,bytes_a_enviar,0);
	free(a_enviar);
	
}

//AUXILIAR
void* armado_a_enviar(op_code codigo_operacion, t_buffer* buffer)
{

	int bytes_a_enviar = buffer->size + 2* sizeof(uint32_t);

	void* a_enviar = malloc(bytes_a_enviar);
  
	int offset = 0;
	memcpy(a_enviar + offset, &(codigo_operacion), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, &(buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset,buffer->stream, buffer->size);
	free(buffer->stream);
  	free(buffer);

	return a_enviar;
}

