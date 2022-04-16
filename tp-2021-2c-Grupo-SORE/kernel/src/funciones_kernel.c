#include "funciones_kernel.h"

void iniciar_logger(void)
{
	
	logger=log_create("./cfg/kernel.log","KERNEL",1,LOG_LEVEL_DEBUG);
	logger_aux=log_create("./cfg/kernel-msj.log","KERNEL",0,LOG_LEVEL_DEBUG);;
    
}


void escucharConexiones() {
    pthread_t threadId[MAX_CONEXIONES];
    int socketDelCliente[MAX_CONEXIONES];
    int contadorConexiones = 0;
    while (1) {
        socketDelCliente[contadorConexiones] = esperar_cliente(server_fd);

        if (socketDelCliente >= 0) {

			//log_info(logger, "Se ha aceptado una conexion: %i\n",socketDelCliente[contadorConexiones]);

			if ((pthread_create(&threadId[contadorConexiones], NULL,   procesar_mensajes,
					 (void*) &socketDelCliente[contadorConexiones])) < 0) {
				//hay un error
				
               
			} else {
				//log_info(logger, "Se recibio a un nuevo carpincho!\n");
				pthread_detach(threadId[contadorConexiones]);
			}
			
		} else {
			log_error(logger, "Falló al aceptar conexión de un nuevo carpincho.");
		}

		contadorConexiones++;
    }    
}


void procesar_mensajes(int* socket_conectado) 
{
    
	int socket = *(int*) socket_conectado;
	op_code codigo_operacion = recibir_operacion(socket);
	t_buffer* buffer=crear_tbuffer();
	recibir_buffer(socket, buffer);
	
		switch (codigo_operacion) {
		case INIT:
			log_info(logger_aux, "Mensaje Recibido: INIT \n");
			procesar_init(buffer,socket);
			break;
		case CLOSE:
			log_info(logger_aux, "Mensaje Recibido: CLOSE \n");
			procesar_close(buffer);
			break;
		case MEMALLOC:
			log_info(logger_aux, "Mensaje Recibido: MEMALLOC \n");
			procesar_memalloc(buffer, socket);
			procesar_mensajes(&socket);
			break;
		case MEMFREE:
			log_info(logger_aux, "Mensaje Recibido: MEMFREE \n");
			procesar_memfree( buffer,  socket);
			procesar_mensajes(&socket);
			break;
		case MEMREAD:
			log_info(logger_aux, "Mensaje Recibido: MEMREAD \n");
			procesar_memread(buffer, socket);
			procesar_mensajes(&socket);
			break;	
		case SEM_INIT:
			log_info(logger_aux, "Mensaje Recibido: SEM_INIT \n");
			procesar_seminit(buffer, socket);
			procesar_mensajes(&socket);
			break;
		case SEM_WAIT:
			log_info(logger_aux, "Mensaje Recibido: SEM_WAIT \n");
			procesar_semwait(buffer, socket);
			break;
		case SEM_POST:
			log_info(logger_aux, "Mensaje Recibido: SEM_POST \n");
			procesar_sempost(socket, buffer);
			procesar_mensajes(&socket);
			break;
		case SEM_DESTROY:
			log_info(logger_aux, "Mensaje Recibido: SEM_DESTROY \n");
			procesar_semdestroy(socket, buffer);
			procesar_mensajes(&socket);
			break;			
		case CALL_IO:
			log_info(logger_aux, "Mensaje Recibido: CALL_IO \n");
			procesar_callio(socket,buffer);
			break;		
		case MEMWRITE:
			log_info(logger_aux, "Mensaje Recibido: MEMWRITE \n");
			procesar_memwrite(socket, buffer);
			procesar_mensajes(&socket);
			break;	
		case -1:
			log_error(logger, "El cliente se desconecto. (Socket %i)", socket);
			return (void*)EXIT_FAILURE;
		default:
			log_warning(logger_aux,"Operacion desconocida.");
			break;
		}

}


void procesar_init( t_buffer* buffer, int socket_carpincho)
{
	t_msj_init* msj_matelib = buffer_unpack_init( buffer);

	t_pcb* new_pcb = init_pcb();
	new_pcb->conexion = socket_carpincho;
	//log_debug(logger_aux, "Carpincho %i Socket %i",new_pcb->pid, new_pcb->conexion);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO INIT.", new_pcb->pid);	

	//Envio a memoria:
	//TODO Pensar si HAY ALGUNA POSIBILIDAD DE QUE NO SE PUEDA PROCESAR EL INIT
	int socket_memoria = conectar_con_memoria();
  	enviar_init(new_pcb->pid, socket_memoria);
	e_status status= recibir_status(socket_memoria);
	log_info(logger_aux, "INIT  Procesado id: %i Status recibido de memoria: %s",new_pcb->pid,status_to_string(status) );	
	liberar_conexion(socket_memoria);

	//Preparo el mensaje para enviar a la matelib(Se va a enviar cuando este en EXEC):
	new_pcb->envios_pendientes= true; 
	t_buffer* msj= buffer_pack_init_ml(new_pcb->pid, status);
	new_pcb->envio_pendiente = msj;
	
	agregar_carpincho_a_list(ESTADO_NEW, new_pcb, mutex_new);
	sem_post(&sem_new);
	
	free(msj_matelib);
}


void procesar_memalloc(t_buffer* buffer, int socket_carpincho)
{

	t_msj_memalloc *msj= buffer_unpack_memalloc( buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO MEMALLOC (Tamanio %i).", msj->pid, msj->tamanio);	

	//Reenvio los datos a memoria:
	int socket_memoria= conectar_con_memoria();
	enviar_memalloc(msj, socket_memoria);

	//Recibo la direccion de memoria.
	//todo: Ver (podria pasar que no se procese el memalloc?) tengo que loguear algun status?
	int address;
	recv(socket_memoria, &address,sizeof(uint32_t),MSG_WAITALL);

	log_info(logger, "[CARPINCHO %i] MENSAJE MEMALLOC. Respuesta Memoria: Address: %i ", msj->pid, address);
	liberar_conexion(socket_memoria);
	

	//Contesto a la matelib:
	send(socket_carpincho, &address ,sizeof(uint32_t),0);
	log_info(logger, "[CARPINCHO %i] MENSAJE MEMALLOC procesado. Respuesta Memoria: Address: %i ", msj->pid, address);
	
}


void mostrar_todas_las_listas()
{
	printf("Carpinchos en NEW\n");
	mostrar_carpinchos(ESTADO_NEW);
	printf("Carpinchos en READY\n");
	mostrar_carpinchos(ESTADO_READY);
	printf("Carpinchos en EXEC\n");
	mostrar_carpinchos(ESTADO_EXEC);
	printf("Carpinchos en BLOCKED\n");
	mostrar_carpinchos(ESTADO_BLOCKED);
	printf("Carpinchos en SUSPENDED BLOCKED\n");
	mostrar_carpinchos(ESTADO_SUSPENDED_BLOCKED);
	printf("Carpinchos en SUSPENDED READY\n");
	mostrar_carpinchos(ESTADO_SUSPENDED_READY);
	//printf("Carpinchos en EXIT\n");
	//mostrar_carpinchos(ESTADO_EXIT);
}


void procesar_close(t_buffer* buffer)
{
	
	int pid = buffer_unpack_close(buffer);//pid carpincho
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO MATE_CLOSE.", pid);	

	t_pcb* carpincho= buscar_carpincho_de_id(ESTADO_EXEC, pid);
	desalojar_carpincho(pid);
	carpincho->estado=EXIT;
	sem_post(&sem_multiprogramacion);
	log_info(logger, "[CARPINCHO %i] CAMBIO DE ESTADO: EXIT", carpincho->pid);
	
	//Aviso a memoria y recibo el status.
	int socket_memoria= conectar_con_memoria();
	enviar_close(carpincho->pid, socket_memoria);
	e_status status= recibir_status(socket_memoria);

	log_info(logger_aux, "[CARPINCHO %i] MATECLOSE PROCESADO", carpincho->pid);
	
	//Aviso al carpincho que ya procese su solicitud.
	enviar_status(status,carpincho->conexion );
	liberar_conexion(socket_memoria);
	
	//Libero todos los recursos. 
	liberar_carpincho(carpincho);
	//mostrar_todas_las_listas();
}

t_pcb* buscar_carpincho_de_id(t_list* lista, int id)
{
	
	bool es_de_id(void* arg){
		t_pcb *carpincho = (t_pcb*)arg;
		return (carpincho->pid == id);
	}	
	return (t_pcb*) list_find(lista, es_de_id);

}


void liberar_carpincho(t_pcb* carpincho)
{

	//Libero los recursos que puede tener asignado al carpincho:
	while(!list_is_empty(carpincho->recursos_asignados))
	{
		t_sem_uso* semaforo = list_remove(carpincho->recursos_asignados,0);
		t_semaforo* semaforo_global = buscar_semaforo_de_nombre(semaforo->nombre);

		for(int i=0; i<semaforo->cantidad; i++){
			t_pcb* proximo_carpincho_a_ejecutar = hacer_post(semaforo_global);
			log_info(logger, "[CARPINCHO %i] Liberando recursos asignados al carpincho (%s) - Se desbloquea carpincho %i", carpincho->pid,  semaforo_global->nombre, proximo_carpincho_a_ejecutar->pid);
			preparar_mensaje_a_enviar(proximo_carpincho_a_ejecutar, STATUS_SUCCESS);
			agregar_carpincho_a_semaforo(semaforo_global, proximo_carpincho_a_ejecutar);
			agregar_semaforo_a_carpincho(semaforo_global->nombre, proximo_carpincho_a_ejecutar);
			desbloquear_carpicho(proximo_carpincho_a_ejecutar);
		}

		free(semaforo->nombre);
		free(semaforo);

		//Saco la  referencia en el semaforo global del carpincho
		sacar_si_esta_en_lista(carpincho, semaforo_global->carpinchos_en_uso, semaforo_global->mutex_sem);
	}
	
	free(carpincho->recursos_asignados);
	free(carpincho->ultimo_tiempo_inicio_exec);
	free(carpincho->ultimo_tiempo_inicio_ready);
	//log_debug(logger, "Libero carpincho %i conexion %i", carpincho->pid, carpincho->conexion);
	liberar_conexion(carpincho->conexion);
	// if(carpincho->envio_pendiente->stream != NULL){free(carpincho->envio_pendiente->stream);};
	//if(carpincho->envio_pendiente!= NULL){free(carpincho->envio_pendiente);}
	free(carpincho);	
}


int generarId()
{
	pthread_mutex_lock(&mutex_pid_counter);
	pid_counter++;
	int proximo_pid= pid_counter;
	pthread_mutex_unlock(&mutex_pid_counter);
	return proximo_pid;
}


t_pcb* init_pcb()
{
	t_pcb* new_pcb=malloc(sizeof(t_pcb));
	new_pcb->pid=generarId();
	new_pcb->estado=NEW;
	log_info(logger, "[CARPINCHO %i] CAMBIO DE ESTADO : NEW ", new_pcb->pid);
	new_pcb->rafaga_real_anterior=0;
	new_pcb->rafaga_estimada= estimacion_inicial;
	new_pcb->RR=0;
	new_pcb->ultimo_tiempo_inicio_exec="";
	new_pcb->ultimo_tiempo_inicio_ready="";
	new_pcb->recursos_asignados=list_create();
	new_pcb->envios_pendientes=false;
	new_pcb->envio_pendiente=NULL;

	return new_pcb;

}


void inicializar()
{
 	config= config_create("./cfg/kernel.config");

	if(config==NULL){
		log_error(logger,"[CONFIGURACION] No se pudo leer el archivo de cofiguración.");
		exit(1);
	}
	char* ip =string_duplicate(config_get_string_value(config, "IP"));
	log_info(logger,"[CONFIGURACION] IP  servidor: %s", ip);
	char* puerto =string_duplicate(config_get_string_value(config, "PUERTO"));
	log_info(logger,"[CONFIGURACION] Puerto  servidor: %s", puerto);
	server_fd = iniciar_servidor(ip,puerto);
	if(server_fd>0){
		log_info(logger, "[CONFIGURACION] Servidor listo para recibir a los nuevos carpinchos");
	}else{
		log_error(logger, "[CONFIGURACION] No se pudo crear el servidor.");	
		exit(1);
	}

	algoritmo=string_duplicate(config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
	estimacion_inicial=config_get_double_value(config, "ESTIMACION_INICIAL");
	log_info(logger,"[CONFIGURACION] Configuracion Algoritmo planificacion corto plazo: %s", algoritmo);
	log_info(logger,"[CONFIGURACION] Configuracion estimacion inicial: %f", estimacion_inicial);
	alfa=config_get_double_value(config, "ALFA");
	log_info(logger,"[CONFIGURACION] Configuracion alfa: %f", alfa);
	tiempo= config_get_int_value(config, "TIEMPO_DEADLOCK");
	log_info(logger,"[CONFIGURACION] Tiempo Deadlock: %i", tiempo);
	cantidadCPU=config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");
	log_info(logger,"[CONFIGURACION] Configuracion grado de multiprocesamiento: %i", cantidadCPU);
	int grado_multiprogramacion= config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
	log_info(logger, "[CONFIGURACION] Configuracion grado de multiprogramacion: %i", grado_multiprogramacion);
	pid_counter=0;

	//	INICIALIZAR LISTAS
	ESTADO_NEW=list_create();
	ESTADO_READY=list_create();
	ESTADO_SUSPENDED_READY=list_create();
	ESTADO_BLOCKED=list_create();
	ESTADO_SUSPENDED_BLOCKED=list_create();
	ESTADO_EXEC=list_create();
	ESTADO_EXIT=list_create();
	SEMAFOROS=list_create();
	DISPOSITIVOS_IO= list_create();

	//INICIALIZAR MUTEX
	pthread_mutex_init(&mutex_new, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_suspended_ready, NULL);
	pthread_mutex_init(&mutex_blocked, NULL);
	pthread_mutex_init(&mutex_suspended_blocked, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
	pthread_mutex_init(&mutex_exit, NULL);
	pthread_mutex_init(&mutex_pid_counter, NULL);
	pthread_mutex_init(&mutex_dispositivosIO, NULL);
	pthread_mutex_init(&mutex_semaforos, NULL);

	//INICIALIZO SEMAFOROS
	sem_init(&sem_multiprogramacion,0,grado_multiprogramacion);
	sem_init(&sem_new,0,0);
	sem_init(&sem_ready,0,0);
	
	cargar_configuracion_inicial_dispositivosIO();
		
}


void cargar_configuracion_inicial_dispositivosIO()
{

	char** dispositivos_io = (char**)config_get_array_value(config,"DISPOSITIVOS_IO"); 
	
	char** duraciones_io = (char**) config_get_array_value(config,"DURACIONES_IO");
	
	int indice=0;
	do{
		t_dispositivoIO* dispositivo=malloc(sizeof(t_dispositivoIO));
		dispositivo->nombre=dispositivos_io[indice];
		dispositivo->duracion=atoi(duraciones_io[indice]);
		dispositivo->queue_carpinchos_en_espera=queue_create(); //Carpinchos que estan esperando el acceso
		indice++;
		list_add(DISPOSITIVOS_IO, dispositivo);
		//log_info(logger, "Se cargo la configuracion inicial del dispositivo IO %i  nombre:%s duracion: %i",indice,dispositivo->nombre, dispositivo->duracion);
	}while(dispositivos_io[indice]!=NULL);

	free(dispositivos_io);
	free(duraciones_io);

	//Creo un hilo para administrar cada dispositivo IO:
	pthread_t hilo_dispositivos_io[indice-1];
	for(int i=0; i< indice; i++){
		t_dispositivoIO* dispositivoIO = list_get(DISPOSITIVOS_IO, i);
		pthread_create(&hilo_dispositivos_io[i], NULL, (void*) &administrar_IO, (void*)dispositivoIO);
		pthread_detach(hilo_dispositivos_io[i]);
	}
}

void* administrar_IO(t_dispositivoIO* dispositivoIO){
	log_info(logger, "[CONFIGURACION] Dispositivo de  IO  Creado((%s - duracion: %i))", dispositivoIO->nombre,dispositivoIO->duracion);

    dispositivoIO->queue_carpinchos_en_espera = queue_create();

    sem_t sem_io_queue;
    dispositivoIO->sem_carpincho_en_cola = malloc(sizeof(sem_t));
    dispositivoIO->sem_carpincho_en_cola = &sem_io_queue;
    sem_init(&sem_io_queue, 0, 0);

    sem_t sem_io_ejecucion;
    dispositivoIO->sem_termino_de_ejecutar = malloc(sizeof(sem_t));
    dispositivoIO->sem_termino_de_ejecutar = &sem_io_ejecucion;
    sem_init(&sem_io_ejecucion, 0, 1);

    while (1)
	{
        sem_wait(&sem_io_queue);
        sem_wait(&sem_io_ejecucion);
		t_pcb* carpincho = queue_pop(dispositivoIO->queue_carpinchos_en_espera);
		log_info(logger, "[CARPINCHO %i] Dispositivo %s Empezó a  ejecutar", carpincho->pid, dispositivoIO->nombre );
		usleep(dispositivoIO->duracion*1000);
		sem_post(dispositivoIO->sem_termino_de_ejecutar);
		log_info(logger, "[CARPINCHO %i] Dispositivo %s Terminó de ejecutar ", carpincho->pid, dispositivoIO->nombre);
		desbloquear_carpicho(carpincho);
	}
  
}

void desbloquear_carpicho(t_pcb* carpincho)
{
	if(esta_suspendido(carpincho))
	{
		agregar_a_suspendidoReady(carpincho);

	}else
	{	
		sacar_si_esta_en_lista(carpincho, ESTADO_BLOCKED,mutex_blocked );
		agregar_carpincho_a_ready(carpincho);
	}		
	
}


bool esta_suspendido(t_pcb* carpincho)
{
	t_pcb* buscado = buscar_carpincho_de_id(ESTADO_SUSPENDED_BLOCKED,carpincho->pid);

	return	buscado !=NULL;
}


void agregar_a_suspendidoReady(t_pcb* carpincho)
{
	pthread_mutex_lock(&mutex_suspended_blocked);
	int indice= buscarIndice(carpincho, ESTADO_SUSPENDED_BLOCKED);
	if(indice <0){
		log_error(logger,"[CARPINCHO %i] CARPINCHO NO ESTA EN SUSPENDIDO LISTO", carpincho->pid );
	}else{
		log_info(logger, "[CARPINCHO %i] CAMBIO DE ESTADO: SUSPENDIDO LISTO", carpincho->pid );
		list_remove(ESTADO_SUSPENDED_BLOCKED,indice );
		agregar_carpincho_a_list(ESTADO_SUSPENDED_READY, carpincho, mutex_suspended_ready);
		sem_post(&sem_new);
	}	
	pthread_mutex_unlock(&mutex_suspended_blocked);

}

void destruir_listas(){
	list_destroy(ESTADO_NEW);
	list_destroy(ESTADO_READY);
	list_destroy(ESTADO_SUSPENDED_READY);
	list_destroy(ESTADO_BLOCKED);
	list_destroy(ESTADO_SUSPENDED_BLOCKED);
	list_destroy(ESTADO_EXEC);
	list_destroy(ESTADO_EXIT);
	list_destroy_and_destroy_elements(ESTADO_EXEC,liberar_carpincho);

}

void destruir_mutex(){
	pthread_mutex_destroy(&mutex_new);
	pthread_mutex_destroy(&mutex_ready);
	pthread_mutex_destroy(&mutex_suspended_ready);
	pthread_mutex_destroy(&mutex_blocked);
	pthread_mutex_destroy(&mutex_suspended_blocked);
	pthread_mutex_destroy(&mutex_exec);
	pthread_mutex_destroy(&mutex_pid_counter);

}


void finalizar()
{
	log_info(logger,"Kernel finalizando.");
	//TODO Ver de destruir los dispositivos IO y semaforos.
	log_destroy(logger);
	log_destroy(logger_aux);
	config_destroy(config);
	destruir_listas();
	destruir_mutex();
	close(server_fd);
}


void* administrar_multiprogramacion()
{
	// PLANIFICADOR DE LARGO PLAZO
	while(1){
		// Este planificador usará el algoritmo FIFO.
		sem_wait(&sem_multiprogramacion);
		sem_wait(&sem_new);
		t_pcb* carpincho;
		if(!list_is_empty(ESTADO_SUSPENDED_READY)){
			carpincho= encontrar_primer_carpincho(ESTADO_SUSPENDED_READY, mutex_suspended_ready);		
		}else{
			carpincho= encontrar_primer_carpincho(ESTADO_NEW, mutex_new);				
		}
		agregar_carpincho_a_ready(carpincho);
	}
}


void administrar_multiprocesamiento(){
	
	for (int i=0; i < cantidadCPU; i++) {
		 
		if (pthread_create(&threads_CPU[i], NULL, (void*) &planificar_carpinchos,i)) {
			log_warning(logger,"No se pudo crear el hilo %i para administrar el multiprocesamiento \n", i);
		}
		pthread_detach(threads_CPU[i]);
	}	
}


void planificar_carpinchos(int indice){
	
	// PLANIFICADOR DE CORTO PLAZO PLAZO
	if(strcmp( algoritmo,"SJF")==0){
	//log_info(logger, "CPU %i preparada para planificar - ALGORITMO: SJF", indice);

	//TODO MEJORAR ESTO, EL CODIGO ESTA REPETIDO....
		while(1){
			sem_wait(&sem_ready);

			t_pcb* carpincho= encontrar_carpincho_SJF(ESTADO_READY, mutex_ready);
			agregar_carpincho_a_list(ESTADO_EXEC, carpincho,mutex_exec);
			carpincho->estado=EXEC;
			//carpincho->ultimo_tiempo_inicio_exec=temporal_get_string_time("%H:%M:%S:%MS");
			actualizar_tiempo_inicio_exec( carpincho);
			log_info(logger,"[CARPINCHO %i] CAMBIO DE ESTADO: EXEC (CPU  %i)", carpincho->pid,  indice);
			verificar_envios_pendientes(carpincho);
			procesar_mensajes(&carpincho->conexion);
		}
	}
	else if(strcmp(   string_duplicate(config_get_string_value(config, "ALGORITMO_PLANIFICACION")),"HRRN")==0)
	{
		while(1){
			//ALGORITMO HRRN:

			sem_wait(&sem_ready);
	

			t_pcb* carpincho= encontrar_carpincho_HRRN(ESTADO_READY, mutex_ready);
			agregar_carpincho_a_list(ESTADO_EXEC, carpincho,mutex_exec);
			carpincho->estado=EXEC;
			actualizar_tiempo_inicio_exec( carpincho);
			//carpincho->ultimo_tiempo_inicio_exec=temporal_get_string_time("%H:%M:%S:%MS");
			log_info(logger,"[CARPINCHO %i] CAMBIO DE ESTADO: EXEC (CPU  %i)", carpincho->pid,  indice);
			verificar_envios_pendientes(carpincho);
			procesar_mensajes(&carpincho->conexion);
		}
	}	
}


int conectar_con_memoria(){
	
	return crear_conexion(
		config_get_string_value(config, "IP_MEMORIA"),config_get_string_value(config, "PUERTO_MEMORIA"));
	
}


void calcular_rafaga_estimada(t_pcb* carpincho){
    carpincho->rafaga_estimada=  
	( alfa * carpincho->rafaga_real_anterior ) +  ( (1-alfa) * carpincho->rafaga_estimada);
}


t_pcb *buscar_carpincho_con_menor_rafaga(t_list *carpinchos) {
	t_pcb *menorRafaga;
	switch (list_size(carpinchos)) {
		case 0: {
			log_error(logger, "No hay carpinchos para ejecutar!");
			menorRafaga = NULL;
			break;
		}
		case 1: {
			//log_info(logger,"Hay un solo carpincho");
			menorRafaga = (t_pcb*) list_remove(carpinchos, 0);
			//log_debug(logger,"Estimacion Rafaga carpincho %d : %f",menorRafaga->pid, menorRafaga->rafaga_estimada);
			break;
		}
		default: {
			t_pcb *aux1;
			t_pcb *aux2;
			int posicion = 0;
			aux1 = list_get(carpinchos, 0);

			for (int i = 1; i <list_size(carpinchos); i++) {
				aux2 = list_get(carpinchos, i);
				//log_debug(logger,"Estimacion Rafaga carpincho %d : %f",aux2->pid,aux2->rafaga_estimada);
				if (aux1->rafaga_estimada > aux2->rafaga_estimada) {
					aux1 = aux2;
					posicion = i;
				}
			}
			menorRafaga = (t_pcb*) list_remove(carpinchos, posicion);
		}
	}
	return menorRafaga;
}


t_pcb *buscar_carpincho_con_mayor_RR(t_list *carpinchos) {
	t_pcb *mayorRR;
	switch (list_size(carpinchos)) {
		case 0: {
			log_error(logger, "No hay carpinchos para ejecutar!");
			mayorRR = NULL;
			break;
		}
		case 1: {
			//log_info(logger,"Hay un solo carpincho");
			mayorRR = (t_pcb*) list_remove(carpinchos, 0);
			//log_debug(logger,"Estimacion Rafaga carpincho %d : %f",mayorRR->pid, mayorRR->rafaga_estimada);
			break;
		}
		default: {
			t_pcb *aux1;
			t_pcb *aux2;
			int posicion = 0;
			aux1 = list_get(carpinchos, 0);

			for (int i = 1; i < list_size(carpinchos); i++) {
				aux2 = list_get(carpinchos, i);
				//log_debug(logger,"RR carpincho %d : %f",aux2->pid,aux2->RR);
				if (aux1->RR < aux2->RR) {
					aux1 = aux2;
					posicion = i;
					log_error(logger, "Encontre uno mas grande, posicion %i, pid: %i, RR: %f", i, aux2->pid, aux2->RR);
				}
			}
			mayorRR = (t_pcb*) list_remove(carpinchos, posicion);
		}
	}
	return mayorRR;
}


void actualizar_tiempo_en_exec(t_pcb* carpincho){

	char * tiempo_actual= temporal_get_string_time("%H:%M:%S:%MS");
	carpincho->rafaga_real_anterior = get_timestamp_diff(tiempo_actual, carpincho->ultimo_tiempo_inicio_exec);
	free(tiempo_actual);
	//carpincho->rafaga_real_anterior=(time(NULL) - carpincho->ultimo_tiempo_inicio_exec)*1000;
	//log_debug(logger,"ACTUALIZO TIEMPO EN EXEC Carpincho: %i      Tiempo: %f \n", carpincho->pid, carpincho->rafaga_real_anterior);
}



bool tiene_mayor_RR(t_pcb* carpincho1, t_pcb* carpincho2){
    return (carpincho1->RR) >= (carpincho2->RR);
}


void actualizar_RR(t_pcb* carpincho){
	// RR= 1 + ( W / S ) 
	char* tiempo_actual= temporal_get_string_time("%H:%M:%S:%MS");
	double waiting= get_timestamp_diff(  tiempo_actual,    carpincho->ultimo_tiempo_inicio_ready ) ;
	free(tiempo_actual);

	if(carpincho->rafaga_estimada == 0)
	{
		carpincho->RR = 1;
	}else
	{
		carpincho->RR =	1+( waiting / carpincho->rafaga_estimada);
	}
	//log_debug(logger, "ACTUALIZACION DE RR: Carpincho %i  Waiting: %f  Rafaga Estimada: %f  RR :%f \n",  carpincho->pid, waiting, carpincho->rafaga_estimada, carpincho->RR);
	
}


void mostrar_carpincho(t_pcb* carpincho)
{
    //mostrar_estado(carpincho->estado);
	log_debug(logger,"[CARPINCHO %i] RAFAGA REAL ANT: %f    RAFAGA ESTIM: %f    RR: %f\n",
		carpincho->pid,
		carpincho->rafaga_real_anterior,
		carpincho->rafaga_estimada, 
		carpincho->RR);
}


void mostrar_carpinchos(t_list *carpinchos)
{
	if(!list_is_empty(carpinchos))
		list_iterate(carpinchos,mostrar_carpincho);
	else
	printf("---------");
}


void mostrar_estado(t_status estado) {
	if (estado == NEW) {
		printf("NEW ");
	}

	if (estado == READY) {
		printf("READY ");
	}

	if (estado == BLOCKED) {
		printf("BLOCKED ");
	}

	if (estado == SUSPENDED_BLOCKED) {
		printf("SUSPENDED_BLOCKED ");
	}

	if (estado == EXEC) {
		printf("EXEC ");
	}
	if (estado == EXIT) {
		printf("EXIT ");
	}

}


void procesar_memfree(t_buffer* buffer, int socket)
{	
	t_msj_memfree* msj= buffer_unpack_memfree(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO MATE_MEMFREE (Address %i).", msj->pid, msj->add);

	//Reenvio el mensaje a memoria y recibo el status
	int socket_memoria= conectar_con_memoria();
	enviar_memfree(msj, socket_memoria);
	e_status status= recibir_status(socket_memoria);
	
	//Contesto al carpincho:
	enviar_status( status,  socket ); 	
	log_info(logger_aux, "[CARPINCHO %i] Mensaje MEMFREE procesado - Status: %s", msj->pid ,status_to_string(status));

}
	

void procesar_memread(t_buffer* buffer, int socket)
{
	
	t_msj_memread* msj= buffer_unpack_memread(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO MATE_MEMREAD ", msj->pid);

	//Reenvio el mensaje a memoria y recibo el status
	int socket_memoria= conectar_con_memoria();
	enviar_memread(msj, socket_memoria);

	t_buffer *buffer_rta =  crear_tbuffer();
  	recibir_buffer(socket_memoria, buffer_rta);
	log_info(logger,"[CARPINCHO %i] RESPUESTA RECIBIDA DE MEMORIA MATE_MEMREAD ", msj->pid);
	
	//Contesto al carpincho:
	enviar_buffer(socket, buffer_rta );	
	log_info(logger_aux, "[CARPINCHO %i] Mensaje MEMREAD procesado", msj->pid );

}

void procesar_seminit(t_buffer* buffer, int socket)
{

	t_msj_seminit* msj= buffer_unpack_seminit(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO SEM_INIT ( %s value: %i).", msj->pid, msj->nombre, msj->value);

	inicializar_semaforo_si_no_esta_inicializado(msj->nombre, msj->value);

	//Respondo al carpincho
	t_status status= STATUS_SUCCESS;
	enviar_status( status,  socket ); //TODO Pensar En que caso no seria success??	
	log_info(logger_aux, "[CARPINCHO %i] Mensaje SEM_INIT procesado status: %s", msj->pid,status_to_string(status));
	
	free(msj);

}


void procesar_semwait(t_buffer* buffer, int socket)
{

	t_msj_semwait* msj= buffer_unpack_semwait(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO SEM_WAIT ( %s ).", msj->pid, msj->nombre);

	t_semaforo* sem=  buscar_semaforo_de_nombre(msj->nombre);
	t_pcb* carpincho= buscar_carpincho_de_id(ESTADO_EXEC,msj->pid);
	

	if(sem!=NULL)
	{
		pthread_mutex_lock(&sem->mutex_sem);
		sem->valor--;
	
		if(sem->valor<0)
		{
			log_info(logger, "[CARPINCHO %i] SEM_WAIT:  NO  SE LE ASIGNA EL SEMAFORO %s (BLOQUEADO)", msj->pid, msj->nombre);
			queue_push(sem->queue_espera,carpincho);
			pthread_mutex_unlock(&sem->mutex_sem);
			pasar_exec_a_blocked(carpincho);
			
		}
		else
		{
			//Se le asigna el semaforo solicitado
			agregar_semaforo_a_carpincho(sem->nombre,carpincho);
			agregar_carpincho_a_semaforo(sem, carpincho);
			pthread_mutex_unlock(&sem->mutex_sem);
			e_status status = STATUS_SUCCESS;
			buffer_pack(buffer, &status, sizeof(uint32_t));
			enviar_buffer(socket, buffer);
			log_info(logger, "[CARPINCHO %i] SEM_WAIT: SE LE ASIGNA EL SEMAFORO %s", msj->pid, msj->nombre);
		
			procesar_mensajes(&socket);
		}	
	}
	else
	{
		log_error (logger,"[CARPINCHO %i] SEM_WAIT: Semaforo inexistente %s", msj->pid, msj->nombre);
		e_status status = STATUS_SEM_INEXISTENTE;
		buffer_pack(buffer, &status, sizeof(uint32_t));
		enviar_buffer(socket, buffer);
	
	}

}


void desalojar_carpincho(int pid_carpincho)
{
	remover_carpincho_de__id(pid_carpincho, ESTADO_EXEC, mutex_exec);
}


void agregar_carpincho_a_semaforo(t_semaforo* sem  , t_pcb* carpincho)
{
	list_add(sem->carpinchos_en_uso, carpincho);
	
}

void agregar_semaforo_a_carpincho(char* nombre_semaforo, t_pcb* carpincho)
{
	bool _es_de_nombre(char* arg) {
		t_sem_uso *semaforo = (t_sem_uso*)arg;
		return !strcmp(semaforo->nombre, nombre_semaforo);
	}

	//Chequea si el carpincho, ya tiene una instancia del semaforo existente, si es asi agrega una nueva instancia.
	//Sino encuentra una, la crea.
	t_sem_uso *semaforo_en_uso = (t_sem_uso*) list_find(carpincho->recursos_asignados, _es_de_nombre);

	if(semaforo_en_uso == NULL)
	{
		t_sem_uso *new_sem_en_uso=malloc(sizeof(t_sem_uso));
		new_sem_en_uso->nombre=string_duplicate(nombre_semaforo);
		new_sem_en_uso->cantidad=1;
		list_add(carpincho->recursos_asignados, new_sem_en_uso);

	}else{
		semaforo_en_uso->cantidad += 1;
	}

}


void pasar_exec_a_blocked(t_pcb* carpincho)
{
	desalojar_carpincho(carpincho->pid);
	actualizar_tiempo_en_exec(carpincho);
	calcular_rafaga_estimada(carpincho);
	agregar_carpincho_a_list(ESTADO_BLOCKED,carpincho, mutex_blocked);
	carpincho->estado=BLOCKED;
	log_info(logger, "[CARPINCHO %i] CAMBIO DE ESTADO : BLOQUEADO", carpincho->pid);
	pthread_mutex_lock(&mutex_blocked);
	pthread_mutex_lock(&mutex_new);
	pthread_mutex_lock(&mutex_ready);
	chequear_suspension();
	pthread_mutex_unlock(&mutex_ready);
	pthread_mutex_unlock(&mutex_new);
	pthread_mutex_unlock(&mutex_blocked);
	
}


void chequear_suspension()
{

	if(!list_is_empty(ESTADO_NEW) && list_is_empty(ESTADO_READY)){
		//Esto significa que todo el grado de multiprogramación esté copado por carpinchos que no están
		// haciendo uso de las CPUs. 
		//Cuando esto suceda, el planificador deberá tomar el último elemento
		// en haber ingresado a la cola de bloqueado y pasarlo a suspendido, para dar entrada a nuevos 
		// carpinchos esperando el grado de multiprogramación en NEW y no generar overhead de CPU en EXEC.
		
		t_pcb* carpincho= list_remove(ESTADO_BLOCKED, list_size(ESTADO_BLOCKED)-1);
		agregar_carpincho_a_list(ESTADO_SUSPENDED_BLOCKED, carpincho, mutex_suspended_blocked);
		log_info(logger, "[CARPINCHO %i] CAMBIO DE ESTADO : SUSPENDIDO BLOQUEADO" , carpincho->pid);
		carpincho->estado = SUSPENDED_BLOCKED;

		//Aviso a memoria para que saque sus paginas.
		int socket_memoria= conectar_con_memoria();
		t_buffer* buffer = crear_tbuffer();
		buffer_pack(buffer, &carpincho->pid, sizeof(uint32_t));
		enviar_mensaje(SUSPENDER,buffer,socket_memoria);
		e_status status= recibir_status(socket_memoria);
		log_info(logger, "[CARPINCHO %i] SUSPENSION Status recibido de memoria: %s",carpincho->pid,status_to_string(status) );	
	

		liberar_conexion(socket_memoria);

		sem_post(&sem_multiprogramacion);
	}
}


void procesar_sempost(int socket, t_buffer* buffer)
{

	t_msj_sempost* msj= buffer_unpack_sempost(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO SEM POST %s.", msj->pid, msj->nombre);
			
	t_semaforo* sem= buscar_semaforo_de_nombre(msj->nombre);
	t_pcb* carpincho_solicitante= buscar_carpincho_de_id(ESTADO_EXEC, msj->pid);

	if(sem!=NULL){

		if(!queue_is_empty(sem->queue_espera)){
			t_pcb* carpincho_en_espera = hacer_post(sem);
			preparar_mensaje_a_enviar(carpincho_en_espera, STATUS_SUCCESS);
			agregar_carpincho_a_semaforo(sem, carpincho_en_espera);
			agregar_semaforo_a_carpincho(sem->nombre, carpincho_en_espera);
			desbloquear_carpicho(carpincho_en_espera);
			log_info(logger,"[CARPINCHO %i] SEM POST %s:  Libera a carpincho %i", carpincho_solicitante->pid, sem->nombre, carpincho_en_espera->pid);
			
		}else{
			pthread_mutex_lock(&sem->mutex_sem);
			sem->valor++;
			pthread_mutex_unlock(&sem->mutex_sem);
		}

		t_buffer* respuesta = crear_tbuffer();
		e_status status = STATUS_SUCCESS;
		buffer_pack(respuesta, &status, sizeof(uint32_t));
		enviar_buffer(carpincho_solicitante->conexion, respuesta);
		log_info(logger,"[CARPINCHO %i] SEM POST %s Procesado: Sigue ejecutando", carpincho_solicitante->pid, sem->nombre);
		remover_carpincho_de_semaforo(sem, carpincho_solicitante);

	
	}
}


void remover_carpincho_de_semaforo(t_semaforo* sem, t_pcb*  carpincho)
{
	int indice= buscarIndice(carpincho,sem->carpinchos_en_uso );
	if(indice ==-1){
		log_warning(logger, "[CARPINCHO %i] SEM_POST Se hizo un post de un semaforo %s no solicitado anteriormente.", carpincho->pid, sem->nombre);
	}else{
		list_remove(sem->carpinchos_en_uso, indice);
	}
	
	for(int i=0; i< list_size(carpincho->recursos_asignados); i++)
	{
		t_sem_uso* semaforo = list_get(carpincho->recursos_asignados,i);
		//log_debug(logger, "[CARPINCHO %i] REMOVER CARPINCHO (RECORRO RR ASIGNADOS) RECURSO %s", carpincho->pid, semaforo->nombre);

		if(strcmp(semaforo->nombre, sem->nombre)==0){
			if(semaforo->cantidad ==1){
				int indice= buscarIndice(semaforo,carpincho->recursos_asignados);
				semaforo= list_remove(carpincho->recursos_asignados, indice);
				free(semaforo->nombre);
				free(semaforo);
			}else{
				semaforo->cantidad--;
			}
			break;
		}
	}
}

void preparar_mensaje_a_enviar(t_pcb* carpincho, e_status status)
{
	t_buffer* buffer= crear_tbuffer();
	buffer_pack(buffer, &status, sizeof(uint32_t));
	carpincho->envios_pendientes=true;
	carpincho->envio_pendiente=buffer;			
}

void procesar_semdestroy(int socket, t_buffer* buffer){

	t_msj_semdestroy* msj = buffer_unpack_semdestroy(buffer);
	log_info(logger, "[CAPINCHO %i] Mensaje Recibido: SEM DESTROY (%s)", msj->pid, msj->nombre);

	t_semaforo* sem= buscar_semaforo_de_nombre(msj->nombre);

	if(sem!= NULL){
		pthread_mutex_lock(&mutex_semaforos);
		int indice= buscarIndice(sem, SEMAFOROS);
		list_remove(SEMAFOROS,indice);
		pthread_mutex_unlock(&mutex_semaforos);
		liberar_semaforo(sem);

		//Contesto al carpincho:
		enviar_status(STATUS_SUCCESS, socket);
		log_info(logger_aux,"[CARPINCHO %i] SEM DESTROY Procesado: CARPINCHO", msj->pid);
		
	}else{
		//Contesto al carpincho:
		enviar_status(STATUS_SEM_INEXISTENTE, socket);
		log_error(logger_aux,"[CARPINCHO %i] SEM DESTROY - Semaforo Inexistente ", msj->pid);
	}

	free(msj);
}


void liberar_semaforo(t_semaforo* sem)
{
	pthread_mutex_destroy(&sem->mutex_sem);
	queue_destroy(sem->queue_espera);
	list_destroy(sem->carpinchos_en_uso);
	free(sem->nombre);
	free(sem);

}


void procesar_callio(int socket, t_buffer* buffer )
{

	t_msj_semcallio* msj=buffer_unpack_callio(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO CALL_IO %s.", msj->pid, msj->nombre);

	t_dispositivoIO * disp_IO = buscar_dispositivoIO_de_nombre(msj->nombre);
	

	if(disp_IO != NULL)
	{	
		t_pcb *carpincho = buscar_carpincho_de_id(ESTADO_EXEC, msj->pid);
		pasar_exec_a_blocked(carpincho);
		queue_push(disp_IO->queue_carpinchos_en_espera,carpincho );
		sem_post(disp_IO->sem_carpincho_en_cola);
		preparar_mensaje_a_enviar(carpincho, STATUS_SUCCESS);
		
	}else{
		e_status status = DEVICE_NOT_FOUND;
		buffer_pack(buffer, &status, sizeof(uint32_t));
		enviar_buffer(socket, buffer);
		procesar_mensajes(&socket);
	}

	free(msj->nombre);
	free(msj);
}

void procesar_memwrite(int socket, t_buffer* buffer)
{
	
	t_msj_memwrite* msj=buffer_unpack_memwrite(buffer);
	log_info(logger,"[CARPINCHO %i] MENSAJE RECIBIDO MEM_WRITE.", msj->pid);

	//Reenvio el mensaje a memoria y recibo el status
	int socket_memoria= conectar_con_memoria();
	enviar_memwrite(msj, socket_memoria);

	e_status status = recibir_status(socket_memoria);
	
	//Contesto al carpincho:
	enviar_status(status, socket);
	log_info(logger, "[CARPINCHO %i] Mensaje MEMWRITE procesado - Status recibido de memoria: %s",msj->pid,status_to_string(status));

}


t_semaforo*  buscar_semaforo_de_nombre(char* nombre_semaforo)
{
	bool _es_de_nombre(char* arg) {
		t_semaforo *semaforo = (t_semaforo*)arg;
		return !strcmp(semaforo->nombre, nombre_semaforo);
	}
	//TODO ver si debo usar el mutex de la lista para buscar el semaforo. 
	t_semaforo* sem = (t_semaforo* ) list_find(SEMAFOROS, _es_de_nombre);

	if( sem  == NULL){
		log_warning(logger, "WARNING No existe sem de nombre %s", nombre_semaforo);
	}

	return sem;
}

t_dispositivoIO*  buscar_dispositivoIO_de_nombre(char* nombre_IO)
{
	bool _es_de_nombre(char* arg) {
		t_dispositivoIO *IO = (t_dispositivoIO*)arg;
		return !strcmp(IO->nombre, nombre_IO);
	}
	//TODO ver si debo usar el mutex de la lista para buscar el semaforo. 
	t_dispositivoIO* disp_IO = (t_dispositivoIO* ) list_find(DISPOSITIVOS_IO, _es_de_nombre);

	if( disp_IO  == NULL){
		log_warning(logger, "WARNING No existe Dispositivo de IO  de nombre %s", nombre_IO);
	}

	return disp_IO;
}


int buscarIndice(void* data, t_list *lista) {
	t_list *aux = list_duplicate(lista);
	int indice = 0;
	int hallado = 0;

	if (!list_is_empty(aux)) {
		while ((aux->head != NULL)
				&& ( (   data !=    (t_pcb*) aux->head->data     ) )) {
			aux->head = aux->head->next;
			indice++;
		}

		if (aux->head == NULL) {
			list_destroy(aux);
			return -1;
		}

		if ( (   data ==    (t_pcb*) aux->head->data     ) ) {
			hallado = 1;
		}
		if (hallado == 1) {
			list_destroy(aux);
			return indice;
		} else {
			list_destroy(aux);
			return -1;
		}

	} else {
		list_destroy(aux);
		return -1;
	}
}


void inicializar_semaforo_si_no_esta_inicializado(char* nombre_semaforo, int valor_inicial){

	
	bool _es_de_nombre(char* arg) {
		t_semaforo *semaforo = (t_semaforo*)arg;
		return !strcmp(semaforo->nombre, nombre_semaforo);
	}
	
	if( list_find(SEMAFOROS,(void*)_es_de_nombre)  == NULL){

		//No existe semaforo con este nombre:creo e inicializo uno nuevo
		t_semaforo *new_sem = malloc(sizeof(t_semaforo));
		new_sem->nombre = string_duplicate(nombre_semaforo);
		new_sem->queue_espera = queue_create();
		pthread_mutex_init(&new_sem->mutex_sem, NULL);
		new_sem->valor=valor_inicial;
		new_sem->carpinchos_en_uso = list_create();
		pthread_mutex_lock(&mutex_semaforos);
		list_add(SEMAFOROS, new_sem);
		pthread_mutex_unlock(&mutex_semaforos);
		//log_info(logger, "Se agrego un nuevo semaforo: %s con valor %i", nombre_semaforo, valor_inicial);
		
	}
	else{
		log_warning(logger, "No se inicializo el semaforo %s con valor %i. Semaforo ya inicializado.", nombre_semaforo, valor_inicial);
		

	}	
}


void detectar_deadlock(){

	while(1)
	{
		usleep(tiempo*1000);
		log_info(logger, "[DEADLOCK] Se inicia  la detección y recuperación de DEADLOCK");
		
		t_list* involucrados= buscar_deadlock();
		if(!list_is_empty(involucrados)){
			resolver_deadlock(involucrados);
		}

		log_info(logger, "[DEADLOCK] Se finaliza la detección y recuperación de DEADLOCK");
		
		
	}
}


t_semaforo* buscar_semaforo_que_lo_bloqueo(t_pcb* carpincho)
{	
	for (int i=0; i<list_size(SEMAFOROS); i++)
	{
		t_semaforo* sem = list_get(SEMAFOROS, i);
		if(esta_en_espera(carpincho, sem))
		{
			return sem;
		}
	}
	return NULL;
}


t_pcb* buscar_carpincho_que_tiene_mi_recurso( t_semaforo* sem)
{
	
	return list_find(sem->carpinchos_en_uso, esta_bloqueado);
}


bool espera_circular(t_list* involucrados, t_pcb* ultimo)
{
	t_semaforo* sem = buscar_semaforo_que_lo_bloqueo(ultimo); //lo que espero

	if(sem!=NULL){
		//log_debug(logger, "El carpincho: %i esta bloqueado por %s" , ultimo->pid, sem->nombre);

		t_pcb* aux= buscar_carpincho_que_tiene_mi_recurso(sem);
		if(aux== NULL){
			list_clean(involucrados);
			return false;
		}else{
			//log_debug(logger, "El carpincho: %i tiene mi recurso asignado %s" , aux->pid, sem->nombre);
			//t_semaforo* sem_aux = buscar_semaforo_que_lo_bloqueo(aux);
			//log_debug(logger, "El carpincho: %i  esta bloqueado por %s" , aux->pid, sem_aux->nombre);
			if(esta_en_la_lista(involucrados, aux)){
				log_info(logger, "[DEADLOCK] ENCONTRE DEADLOCK!!");
				return true;
			}else{
				list_add(involucrados,aux);
				return false;
			}
		}
	}else{
		list_clean(involucrados);
		return false;
	} 	
}


bool esta_en_la_lista(t_list* lista, t_pcb* carpincho)
{
	int id = buscarIndice(carpincho, lista);
	return (id != -1);
}


t_list* deadlock_por_carpincho(t_pcb* carpincho)
{
	//log_debug(logger, "Buscando DEADLOCK para el carpincho: %i", carpincho->pid);
	t_list* posibles_carpinchos_involucrados=list_create();
	t_pcb* proximo= carpincho;
	list_add(posibles_carpinchos_involucrados, proximo);


	if( espera_circular(posibles_carpinchos_involucrados, proximo)  || list_is_empty(posibles_carpinchos_involucrados)){
		//Hay DEADLOCK
		return posibles_carpinchos_involucrados;
	}else{
		proximo= list_get(posibles_carpinchos_involucrados, list_size(posibles_carpinchos_involucrados)-1);
		if(proximo != NULL){
			bool hay_espera_circular= espera_circular(posibles_carpinchos_involucrados, proximo);
			while(  !hay_espera_circular    &&   !list_is_empty(posibles_carpinchos_involucrados)){
			
				proximo= list_get(posibles_carpinchos_involucrados, list_size(posibles_carpinchos_involucrados)-1);
				hay_espera_circular= espera_circular(posibles_carpinchos_involucrados, proximo);
			}
		}
		
		return posibles_carpinchos_involucrados;
	}
}


bool esta_bloqueado_por_semaforo(t_pcb* carpincho)
{
	for(int i=0; i< list_size(SEMAFOROS); i++){
		t_semaforo* sem = list_get(SEMAFOROS, i);
		
		for(int i=0;i<queue_size(sem->queue_espera); i++){
			t_pcb* aux = list_get(sem->queue_espera->elements, i);
			if(carpincho->pid == aux->pid){
				return true;
			} 
		}		
	}
	return false;
}


t_list*  buscar_deadlock()
{
	 	
	pthread_mutex_lock(&mutex_blocked);
	pthread_mutex_lock(&mutex_suspended_blocked);
	t_list* bloqueados = list_create();

	if(!list_is_empty(ESTADO_BLOCKED) || !list_is_empty(ESTADO_SUSPENDED_BLOCKED) ){
		t_pcb* carpincho;
		

		list_add_all(bloqueados, ESTADO_BLOCKED);
		list_add_all(bloqueados, ESTADO_SUSPENDED_BLOCKED);
		
		

		for(int i=0; i<list_size(bloqueados);i++){
			carpincho = list_get(bloqueados,i);
			if(esta_bloqueado_por_semaforo(carpincho)){
				t_list* deadlock= deadlock_por_carpincho(carpincho);
				if(!list_is_empty(deadlock)){
					pthread_mutex_unlock(&mutex_blocked);
					pthread_mutex_unlock(&mutex_suspended_blocked);
					list_destroy(bloqueados);
					return deadlock;
				}
			}
		}
	}
	list_destroy(bloqueados);
	pthread_mutex_unlock(&mutex_blocked);
	pthread_mutex_unlock(&mutex_suspended_blocked);
	t_list* lista_vacia = list_create();
	return lista_vacia;
}


void resolver_deadlock(t_list* carpinchos)
{
	t_pcb* carpincho= buscar_mayor_carpincho(carpinchos);
	log_info(logger, "[DEADLOCK]  Se solicita finalizar al carpincho id %i", carpincho->pid);
	sacar_si_esta_en_lista(carpincho, ESTADO_BLOCKED, mutex_blocked);
	sacar_si_esta_en_lista(carpincho, ESTADO_SUSPENDED_BLOCKED, mutex_suspended_blocked);
	t_semaforo* sem = buscar_semaforo_que_lo_bloqueo(carpincho);
	int indice= buscarIndice(carpincho, sem->queue_espera->elements);
	list_remove(sem->queue_espera->elements,indice);
	//log_debug("Se saca de la cola del sem %s el carpincho %i ", sem->nombre, carpincho_r->pid);
	
	liberar_carpincho(carpincho);

	//Vuelvo a correr el algoritmo para chequear que se soluciono el deadlock:
	t_list* carpinchos_involucrados= buscar_deadlock();
	if( !list_is_empty(carpinchos_involucrados) ){
		resolver_deadlock(carpinchos_involucrados);
	}
}


void sacar_si_esta_en_lista(t_pcb*carpincho, t_list* lista,  pthread_mutex_t mutex){
	pthread_mutex_lock(&mutex);
	int indice = buscarIndice(carpincho, lista);
	if(indice != -1) 
	{
		list_remove(lista, indice);	
	}
	pthread_mutex_unlock(&mutex);
}


t_pcb* buscar_mayor_carpincho(t_list* carpinchos)
{
	int mayor=0;
	t_pcb* carpincho;
	for (int i=0; i<list_size(carpinchos); i++){
		carpincho= list_get(carpinchos,i);
		if(mayor<carpincho->pid){
			mayor=carpincho->pid;
		}
	}
	carpincho= buscar_carpincho_de_id(carpinchos, mayor);
	return carpincho;

}

bool esta_en_espera(t_pcb* carpincho, t_semaforo* sem)
{
	return  buscar_carpincho_de_id(  sem->queue_espera->elements, carpincho->pid) != NULL   ;
	
}

bool esta_bloqueado(t_pcb* carpincho)
{
	t_pcb* buscado = buscar_carpincho_de_id(ESTADO_BLOCKED,carpincho->pid);
	t_pcb* buscado2 = buscar_carpincho_de_id(ESTADO_SUSPENDED_BLOCKED,carpincho->pid);
	return	(buscado !=NULL  || buscado2 !=NULL );
}


char* status[] =
{	"MATE_WRITE_FAULT",//-7
	"MATE_READ_FAULT", //-6
	"MATE_FREE_FAULT", //-5
	"STATUS_SEM_INEXISTENTE", //-4
	"DEVICE_NOT_FOUND",//-3
	"MATE_CONNECTION_FAULT",//-2
	"STATUS_ERROR",//-1
    "STATUS_SUCCESS",//0
};


char* status_to_string(int valor)
{
	return status[valor+7];
}


e_status recibir_status( int socket) 
{
  	e_status status;
	void* recibido=malloc(sizeof(uint32_t));
	recv(socket, recibido,sizeof(uint32_t),MSG_WAITALL);
	memcpy(&status, recibido, sizeof(uint32_t));
	//log_info(logger, "%s recibido", status_to_string (status));
  	free(recibido);
  	return status;
}

t_pcb* hacer_post(t_semaforo* semaforo){
	
	pthread_mutex_lock(&semaforo->mutex_sem);
	semaforo->valor++;
	t_pcb* proximo_carpincho_a_ejecutar= queue_pop(	semaforo->queue_espera);
	pthread_mutex_unlock(&semaforo->mutex_sem);
	return proximo_carpincho_a_ejecutar;
	
}


void agregar_carpincho_a_ready(t_pcb* carpincho)
{
	if(carpincho!= NULL){
		carpincho->estado=READY;
		//sem_wait(&sem_multiprogramacion);
		agregar_carpincho_a_list(ESTADO_READY,carpincho, mutex_ready);
		actualizar_tiempo_inicio_ready(carpincho);
		log_info(logger, "[CARPINCHO %i] CAMBIO DE ESTADO : READY", carpincho->pid);
		sem_post(&sem_ready);

	}
}


void actualizar_tiempo_inicio_ready(t_pcb* carpincho)
{
	char* aux= carpincho->ultimo_tiempo_inicio_ready;
	if(strcmp(aux,"")!=0) free(aux);

	carpincho->ultimo_tiempo_inicio_ready = temporal_get_string_time("%H:%M:%S:%MS");
}

void actualizar_tiempo_inicio_exec(t_pcb* carpincho)
{
	char* aux= carpincho->ultimo_tiempo_inicio_exec;
	if(strcmp(aux,"")!=0) free(aux);

	carpincho->ultimo_tiempo_inicio_exec = temporal_get_string_time("%H:%M:%S:%MS");
}

void verificar_envios_pendientes(t_pcb* carpincho)
{
	//Si el proceso fue desalojado, y quedo pendiente de enviar el status, lo envia.
	if(carpincho->envios_pendientes){

		enviar_buffer(carpincho->conexion, carpincho->envio_pendiente);
		carpincho->envios_pendientes=false;
		log_info(logger, "[CARPINCHO %i] Se envia mensaje pendiente", carpincho->pid);

	}
}


 double get_timestamp_diff(char* timestamp_1, char* timestamp_2) 
 {
    int length_1 = string_length(timestamp_1);
    int length_2 = string_length(timestamp_2);

    if (!length_1 || !length_2) return 0;

	char** timestamp_arr_1 = string_n_split(timestamp_1, 4, ":");
    char** timestamp_arr_2 = string_n_split(timestamp_2, 4, ":");

    int hour_1 = atoi(timestamp_arr_1[0]);
    int min_1 = atoi(timestamp_arr_1[1]);
    int sec_1 = atoi(timestamp_arr_1[2]);
  
    int ms_1 = atoi(timestamp_arr_1[3]);

    int hour_2 = atoi(timestamp_arr_2[0]);
    int min_2 = atoi(timestamp_arr_2[1]);
    int sec_2 = atoi(timestamp_arr_2[2]);
    int ms_2 = atoi(timestamp_arr_2[3]);

    double ms_diff = (hour_1 - hour_2) * 3600000 + (min_1 - min_2) * 60000 + (sec_1 - sec_2) * 1000 + (ms_1 - ms_2);
	
	string_array_destroy(timestamp_arr_1);
	string_array_destroy(timestamp_arr_2);
	
    return ms_diff;
}

