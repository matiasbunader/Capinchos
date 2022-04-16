#include "kernel.h"
#include "funciones_kernel.h"


int main(){
    iniciar_logger();
	inicializar();
	
	threads_CPU = malloc(sizeof(pthread_t) * cantidadCPU);

	pthread_create(&hiloEscucha, NULL, escucharConexiones, NULL); 
    pthread_create(&hiloPlaniLargoPlazo, NULL, administrar_multiprogramacion, NULL);
	pthread_create(&hiloPlaniCortoPlazo, NULL, administrar_multiprocesamiento, NULL);
	pthread_create(&hilo_chequeo_deadlock, NULL, detectar_deadlock, NULL);
	pthread_detach(hiloEscucha);
	pthread_detach(hilo_chequeo_deadlock);
	pthread_join(hiloPlaniLargoPlazo,NULL); 
	pthread_join(hiloPlaniCortoPlazo,NULL); 

	finalizar();
   
}