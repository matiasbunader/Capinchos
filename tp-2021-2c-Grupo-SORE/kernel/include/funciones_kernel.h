#ifndef FUNCIONESKERNEL_H
#define FUNCIONESKERNEL_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <commons/string.h>
#include <stdbool.h>
#include "shared_utils.h"
#include "kernel.h"
#include "sincro.h"
#include "send.h"


void iniciar_logger(void);
void* administrar_Carpinchos(void* socketConectado);
void administrar_multiprocesamiento();
void* administrar_multiprogramacion();
void escucharConexiones();
void pasar_a_blocked(t_pcb* carpincho);

int generarId();
t_pcb* init_pcb();
void inicializar();
void destruir_mutex();
void destruir_listas();
void finalizar();
int conectar_con_memoria();
e_status recibir_status( int socket );
char* status_to_string(int valor);
void desalojar_carpincho(int pid_carpincho);
bool esta_suspendido(t_pcb* carpincho);
t_pcb* buscar_carpincho_de_id(t_list* lista, int id);
bool esta_en_la_lista(t_list* lista, t_pcb* carpincho);
void planificar_carpinchos(int indice);
void actualizar_tiempo_actual();
void actualizar_RR(t_pcb* carpincho);
void actualizar_tiempo_en_exec(t_pcb* carpincho);
void mostrar_carpincho(t_pcb* carpincho);
void mostrar_carpinchos(t_list *carpinchos);
void mostrar_estado(t_status estado);
void calcular_rafaga_estimada(t_pcb* carpincho);
bool tiene_mayor_RR(t_pcb* carpincho1, t_pcb* carpincho2);
bool tiene_menor_rafaga(t_pcb* carpincho1, t_pcb* carpincho2);
void desbloquear_carpicho(t_pcb* carpincho);
void agregar_carpincho_a_ready(t_pcb* carpincho);
void procesar_memwrite(int socket, t_buffer* buffer);
void procesar_seminit(t_buffer* buffer, int socket);
void sacar_si_esta_en_lista(t_pcb*carpincho, t_list* lista,  pthread_mutex_t mutex);
void procesar_semwait(t_buffer* buffer, int socket);
void procesar_sempost(int socket, t_buffer* buffer);
void* administrar_IO(t_dispositivoIO* dispositivoIO);
void procesar_semdestroy(int socket, t_buffer* buffer);
void procesar_close(t_buffer* buffer);
void procesar_callio(int socket, t_buffer* buffer );
void procesar_memalloc(t_buffer* buffer, int socket_carpincho);
void chequear_suspension();
void procesar_init( t_buffer* buffer, int socket_carpincho);
t_semaforo*  buscar_semaforo_de_nombre(char* nombre_semaforo);
t_dispositivoIO*  buscar_dispositivoIO_de_nombre(char* nombre_IO);
void cargar_configuracion_inicial_dispositivosIO();
void pasar_exec_a_blocked(t_pcb* carpincho);
void procesar_mensajes(int* socket_conectado);
t_pcb* hacer_post(t_semaforo* semaforo);
void inicializar_semaforo_si_no_esta_inicializado(char* nombre_semaforo, int valor_inicial);
void agregar_semaforo_a_carpincho(char* nombre_semaforo, t_pcb* carpincho);
void agregar_carpincho_a_semaforo(t_semaforo* sem, t_pcb* carpincho);
void remover_carpincho_de_semaforo(t_semaforo* sem, t_pcb*  carpincho);
t_semaforo*  buscar_si_existe_semaforo_de_nombre(char* nombre_semaforo);
void verificar_envios_pendientes(t_pcb* carpincho);
void procesar_memfree(t_buffer* buffer, int socket);
double get_timestamp_diff(char* timestamp_1, char* timestamp_2);
void detectar_deadlock();
t_list* buscar_deadlock_por_semaforo(t_semaforo* sem);
void resolver_deadlock(t_list* carpinchos);
t_pcb* buscar_mayor_carpincho(t_list* carpinchos);
bool esta_en_espera(t_pcb* carpincho, t_semaforo* sem);
bool esta_bloqueado(t_pcb* carpincho);
bool esta_bloqueado_por_semaforo(t_pcb* carpincho);
t_list*  buscar_deadlock();
t_list* deadlock_por_carpincho(t_pcb* carpincho);
bool espera_circular(t_list* involucrados, t_pcb* ultimo);
t_pcb* buscar_carpincho_que_tiene_mi_recurso( t_semaforo* sem);
t_semaforo* buscar_semaforo_que_lo_bloqueo(t_pcb* carpincho);
void liberar_semaforo(t_semaforo* sem);
void preparar_mensaje_a_enviar(t_pcb* carpincho, e_status status);
void agregar_a_suspendidoReady(t_pcb* carpincho);
void liberar_carpincho(t_pcb* carpincho);
void procesar_memread(t_buffer* buffer, int socket);
void actualizar_tiempo_inicio_ready(t_pcb* carpincho);
void actualizar_tiempo_inicio_exec(t_pcb* carpincho);

#endif
