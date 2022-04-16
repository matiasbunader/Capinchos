#include "kernel.h"

void* armado_a_enviar(op_code codigo_operacion, t_buffer* buffer);





void* buffer_pack_init(uint32_t pid);

t_buffer* buffer_pack_init_ml(uint32_t pid, t_status status);




t_msj_init*  buffer_unpack_init( t_buffer* buffer);
int buffer_unpack_close(t_buffer* buffer);
char* buffer_unpack_string(t_buffer* buffer) ;
t_msj_seminit* buffer_unpack_seminit(t_buffer* buffer);
t_msj_semwait* buffer_unpack_semwait(t_buffer* buffer);
t_msj_semcallio* buffer_unpack_callio(t_buffer* buffer);
t_msj_sempost*  buffer_unpack_sempost(t_buffer* buffer);
t_msj_memfree* buffer_unpack_memfree(t_buffer* buffer);
t_msj_memalloc* buffer_unpack_memalloc(t_buffer* buffer);
t_msj_semdestroy* buffer_unpack_semdestroy(t_buffer* buffer);

void recibir_todo(int sockfd, t_buffer* buffer);



void enviar_status(e_status status, int socket );
void enviar_buffer(int socket,  t_buffer* buffer);
void enviar_mensaje(op_code codigo_operacion, t_buffer* buffer, int socket);
void enviar_memalloc(t_msj_memalloc *msj, int socket);
void enviar_memfree(t_msj_memfree *msj, int socket);
void enviar_init(uint32_t pid, int socket);