#include "sincro.h"
void agregar_carpincho_a_list(t_list* lista, t_pcb* carpincho, pthread_mutex_t mutex)
{
   pthread_mutex_lock(&mutex);
   list_add(lista, carpincho);
   pthread_mutex_unlock(&mutex);

}


t_pcb* encontrar_primer_carpincho(t_list* lista, pthread_mutex_t mutex)
{

   pthread_mutex_lock(&mutex);
   t_pcb* carpincho= list_remove(lista,0);
   pthread_mutex_unlock(&mutex);

   return carpincho;    
}


t_pcb*  encontrar_carpincho_SJF(t_list* lista, pthread_mutex_t mutex)
{
   
    pthread_mutex_lock(&mutex);
    actualizar_RRs(ESTADO_READY, mutex_ready);
	mostrar_carpinchos(ESTADO_READY);
    t_pcb* carpincho = buscar_carpincho_con_menor_rafaga(ESTADO_READY);
    pthread_mutex_unlock(&mutex);

    return carpincho;    
}


t_pcb*  encontrar_carpincho_HRRN(t_list* lista, pthread_mutex_t mutex){
 
    pthread_mutex_lock(&mutex);
    actualizar_RRs(ESTADO_READY, mutex_ready);
	mostrar_carpinchos(ESTADO_READY);

    t_pcb* carpincho = buscar_carpincho_con_mayor_RR(ESTADO_READY);
    pthread_mutex_unlock(&mutex);

    return carpincho;    
}

void actualizar_RRs(t_list* carpinchos, pthread_mutex_t mutex){
    pthread_mutex_lock(&mutex);

	if(!list_is_empty(carpinchos))
    {
	    list_iterate(carpinchos,actualizar_RR);
    }
    pthread_mutex_unlock(&mutex);
}


t_pcb* remover_carpincho_de__id(int pid, t_list* lista, pthread_mutex_t mutex)
{
   pthread_mutex_lock(&mutex);
   t_pcb  *carpincho= buscar_carpincho_de_id(lista, pid);
   int indice = buscarIndice(carpincho, lista);
   if(indice==-1)
   {
       log_error(logger, "ERROR: El carpincho que se desea remover no existe.");
   }
   else
    {
       list_remove(lista,indice);
    }

   pthread_mutex_unlock(&mutex);

   return carpincho;    
}