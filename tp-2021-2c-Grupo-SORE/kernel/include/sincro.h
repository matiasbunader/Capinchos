
#include "kernel.h"
#include "funciones_kernel.h"


void agregar_carpincho_a_list(t_list* lista, t_pcb* carpincho, pthread_mutex_t mutex);
t_pcb*  encontrar_primer_carpincho(t_list* lista, pthread_mutex_t mutex);
t_pcb*  encontrar_carpincho_SJF(t_list* lista, pthread_mutex_t mutex);
t_pcb*  encontrar_carpincho_HRRN(t_list* lista, pthread_mutex_t mutex);
t_pcb* remover_carpincho_de__id(int pid, t_list* lista, pthread_mutex_t mutex);
