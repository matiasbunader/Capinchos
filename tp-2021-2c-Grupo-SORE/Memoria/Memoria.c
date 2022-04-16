#include "utils.h"

int main(void)
{
	inicializarVariables();
	log_info(logger, "Puerto %s", puertoSwamp);

	pthread_create(&hiloServer, NULL, &iniciar_servidor, NULL);

	//pthread_create(&metricas,NULL,sistemaNotificadorEmergencia,NULL);

	pthread_join(metricas, NULL);
	pthread_join(hiloServer, NULL);

	finalizarVariables();
}

void inicializarVariables()
{

	configMemoria = config_create(
		"./configuracionMemoria.config");
	logger = log_create("logger.log", "LOG", 1, LOG_LEVEL_INFO);
	listaDeProcesos = list_create();

	IPswamp = config_get_string_value(configMemoria, "IPSWAMP");
	tamanioPagina = config_get_int_value(configMemoria, "TAMANIOPAGINA");
	puertoSwamp = config_get_string_value(configMemoria, "PUERTOSWAMP");
	IPKernel = config_get_string_value(configMemoria, "IP");
	puertoKernel = config_get_string_value(configMemoria, "PUERTO");
	tamanioMemoria = config_get_int_value(configMemoria, "TAMANIO");
	tipoSwap = config_get_string_value(configMemoria,
									   "ALGORITMO_REEMPLAZO_MMU");
	tipoAsignacion = config_get_string_value(configMemoria, "TIPO_ASIGNACION");
	marcosPorCarpincho = config_get_int_value(configMemoria,
											  "MARCOS_POR_CARPINCHO");
	cantidadEntradasTLB = config_get_int_value(configMemoria,
											   "CANTIDAD_ENTRADAS_TLB");
	algoritmoRemplazoTLB = config_get_string_value(configMemoria,
												   "ALGORITMO_REEMPLAZO_TLB");
	posicionClock = 0;

	senaladorMarcosDisponibles = 0;
	escritura = false;
	globalLRUCLOCK = list_create();
	registroTLB = list_create();
	iniciarListaMarcos();
	conexionSwamp = crear_conexion(IPswamp, puertoSwamp);
	enviarTipoAsignacion(tipoAsignacion);
}

void finalizarVariables()
{
	config_destroy(configMemoria);
	log_destroy(logger);
	list_destroy(listaDeProcesos);
}
void iniciarListaMarcos()
{
	listaDeMarcos = list_create();
	int cantMarcos = tamanioMemoria / tamanioPagina;
	memoriaCompleta = malloc(tamanioMemoria);
	int desplazamiento = 0;
	for (int i = 0; i < cantMarcos; i++)
	{
		Marco *nuevo = malloc(sizeof(Marco));
		nuevo->esLibre = 1;
		nuevo->marco = memoriaCompleta + desplazamiento;
		list_add(listaDeMarcos, nuevo);
		desplazamiento += tamanioPagina;
	}
}
AdmProceso *crearNuevocarpincho(uint32_t pid)
{
	AdmProceso *carpincho = malloc(sizeof(AdmProceso));
	carpincho->id_proceso = pid;
	carpincho->tamanio = 0;
	carpincho->tablaDePaginas = list_create();
	carpincho->hit = 0;
	carpincho->miss = 0;
	if (string_equals_ignore_case("FIJA", tipoAsignacion))
	{
		carpincho->localLRUCLOCK = crearListaLRU();
	}
	else
	{
		carpincho->localLRUCLOCK = list_create();
	}

	carpincho->referencioClock = 0;
	return carpincho;
}

t_list *crearListaLRU()
{
	t_list *listaLRU = list_create();

	for (int i = 0; i < marcosPorCarpincho; i++)
	{
		int *aux = malloc(sizeof(int));
		*aux = senaladorMarcosDisponibles;
		log_info(logger, "El marco asignado encontrado es %d", senaladorMarcosDisponibles);
		list_add(listaLRU, aux);
		senaladorMarcosDisponibles++;
	}
	mostrarListaCLOCKLRU(listaLRU);
	return listaLRU;
}

void mostrarListaCLOCKLRU(t_list *listLRU)
{
	if (string_equals_ignore_case("GLOBAL", tipoAsignacion))
	{
		for (int i = 0; i < list_size(listLRU); i++)
		{

			referenciaGlobalClockLRU *ref = list_get(globalLRUCLOCK, i);
			log_info(logger, "Posicion: %d  Marco:%d ID:%d Pagina: %d", i, ref->marco, ref->id_carpincho, ref->pagina);
		}
	}
	else
	{
		for (int i = 0; i < list_size(listLRU); i++)
		{

			uint32_t *marco = list_get(listLRU, i);
			log_info(logger, "Posicion: %d  Marco:%d", i, *marco);
		}
	}
}

HeapMetadata* controlarHeapPartido(uint32_t dirlog,uint32_t marco,AdmProceso* proceso){
	if(obtenerResto(dirlog) < 9){
		return entreDosPaginas(dirlog,proceso);
	}else{
		void* ptrHeap=memoriaCompleta+(marco*tamanioPagina)+obtenerResto(dirlog);
	}

}
void IniciarProcesoMemoria(void *buffer, int socket)
{
	uint32_t respuesta = 0;
	uint32_t desplazamiento = 0;
	e_remitente remitente;
	memcpy((&remitente), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	log_info(logger, "Recibi el remitente %i", remitente);
	uint32_t pid = 0;
	if (remitente == MATELIB)
	{

		//REMITENTE = MATELIB --> Asigno PID.
		pid += 1;
		//Tengo que comunicarme con matelib y enviarle el pid que genere.
	}
	else if (remitente == KERNEL)
	{

		memcpy(&pid, buffer + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
	}
	if (buscarCarpincho(pid) == NULL)
	{
		AdmProceso *carpincho = crearNuevocarpincho(pid);
		list_add(listaDeProcesos, carpincho);
		log_info(logger, " Se creo el carpincho en memoria");
		uint8_t cod = CREARCARPINCHO;
		void *buffer = malloc(sizeof(uint8_t) + sizeof(uint32_t));
		uint32_t desplazamiento = 0;
		memcpy(buffer, &(cod), sizeof(uint8_t));
		desplazamiento += sizeof(uint8_t);
		memcpy(buffer + desplazamiento, &(pid), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		send(conexionSwamp, buffer, desplazamiento, 0);

		send(socket, &respuesta, sizeof(uint32_t), 0);
	}
	else
	{
		log_info(logger, " el numero de PID de la TLB ya existe");
		respuesta = -1;
		send(socket, &respuesta, sizeof(uint32_t), 0);
	}
}

//------------------------------Funciones Principales para Memalloc-----------------------

void realizarMemalloc(void *buffer, int socket)
{
	Memalloc *recibido = deserializarMemalloc(buffer);
	log_info(logger, "La pid recibida es %d", recibido->pid);
	log_info(logger, "El tamanio a reservar es %d", recibido->size);
	AdmProceso *encontrado = buscarCarpincho(recibido->pid);
	uint32_t dirLog;
	log_info(logger, "Cantidad de paginas %d",
			 list_size(encontrado->tablaDePaginas));

	if (list_size(encontrado->tablaDePaginas) == 0)
	{

		encontrado->tamanio += recibido->size + sizeof(HeapMetadata) * 2;
		if (encontrado->tamanio > tamanioPagina * marcosMaximosSwamp)
		{
			log_info(logger, "ERROR: tamanio a reservar no es posible");
			encontrado->tamanio = 0;
			dirLog = -1;
			send(socket, &dirLog, sizeof(uint32_t), 0);
		}
		else
		{
			log_info(logger, "Se puede realizar la reserva, longitud aceptable");
			dirLog = primeraAsignacion(recibido->size, encontrado);
			log_info(logger, "A enviar address: %i ", dirLog);
			mostrarListaCLOCKLRU(encontrado->localLRUCLOCK);
			//Enviar a Kernel
			log_info(logger, "Cantidad de paginas %d",
					 list_size(encontrado->tablaDePaginas));
			mostrarPaginasTotales(encontrado->tablaDePaginas, encontrado->id_proceso);
			mostrarListaCLOCKLRU(globalLRUCLOCK);
			send(socket, &dirLog, sizeof(uint32_t), 0);
		}
	}
	else
	{

		uint32_t dirEncontrada = hayAllocLibre(recibido->size,
											   encontrado->id_proceso, encontrado->tablaDePaginas);

		if (dirEncontrada != -1)
		{
			log_info(logger, "La direccion logica del marco libre es %d",
					 dirEncontrada);
			
			allocarEnAllocLibre(recibido->size, encontrado, dirEncontrada);

			log_info(logger, "Cantidad de paginas %d",
					 list_size(encontrado->tablaDePaginas));
			

			//Enviar a Kernel
			log_info(logger, "A enviar address: %i", dirEncontrada);
			send(socket, &dirEncontrada, sizeof(uint32_t), 0);
		}
		else
		{
			dirLog = asignarMasPaginas(recibido->size, encontrado->tamanio, encontrado);

			mostrarListaCLOCKLRU(encontrado->localLRUCLOCK);
			log_info(logger, "La direccion logica es %d", dirLog);
			log_info(logger, "Cantidad de paginas %d",
					 list_size(encontrado->tablaDePaginas));
			encontrado->tamanio += recibido->size + sizeof(HeapMetadata);
			mostrarListaTLBs();

			log_info(logger, "Cantidad de paginas %d",
					 list_size(encontrado->tablaDePaginas));
			mostrarPaginasTotales(encontrado->tablaDePaginas, encontrado->id_proceso);
			mostrarListaCLOCKLRU(globalLRUCLOCK);
			//Enviar a Kernel
			log_info(logger, "A enviar address: %i", dirLog);
			send(socket, &dirLog, sizeof(uint32_t), 0);
		}
	}
}

uint32_t asignarMasPaginas(int size, int tamanioActual,
						   AdmProceso *proceso)
{
	uint32_t dirLog;
	uint32_t resto = obtenerResto(tamanioActual);

	log_info(logger, "El tamanio ocupado de esta pagina es %d ", resto);
	log_info(logger, "El resto de  esta pagina es %d ", tamanioPagina - resto);

	void *alloc = crearAlloc(size, tamanioActual - sizeof(HeapMetadata));

	Pagina *inicial = list_get(proceso->tablaDePaginas,
							   obtenerDeterminadaPagina(tamanioActual));

	void *marco = memoriaCompleta + (tamanioPagina * inicial->marco);
	HeapMetadata *comienzo = marco + (resto - sizeof(HeapMetadata));
	comienzo->esLibre = false;
	comienzo->nextAlloc = tamanioActual + size;

	log_info(logger, "El alloc previo de comienzo es %d", comienzo->prevAlloc);
	log_info(logger, "El alloc siguiente de comienzo es %d",
			 comienzo->nextAlloc);

	uint32_t recorrido = 0;
	uint32_t limite = sizeof(HeapMetadata) + size;
	//Verifico que el tamanio a reservar es posible en Swamp
	uint32_t respuesta = 0;
	if (string_equals_ignore_case("GLOBAL", tipoAsignacion) && limite > resto)
	{
		respuesta = enviarConsultaReserva(proceso->id_proceso, obtenerDeterminadaPagina(tamanioActual), limite - resto);
		if (respuesta)
		{
			log_info(logger, "Paginas suficientes en Swamp Global");
		}
		else
		{
			dirLog = -1;
			return dirLog;
		}
	}

	if (limite < tamanioPagina - resto)
	{
		log_info(logger, "Cabe en esta pagina");
		memcpy(marco + resto, alloc + recorrido, limite);
		inicial->size += limite;
		recorrido += limite;
		log_info(logger, "Tamanio ocupado en esta pagina es %d ",
				 inicial->size);
		log_info(logger, "Recorrido en esta pagina es %d ", recorrido);
	}
	else
	{
		if ((tamanioPagina - obtenerResto(comienzo->nextAlloc)) < 9)
		{
			log_info(logger, "El Heap quedara partido entre dos paginas");
			log_info(logger, "La reserva necesita otra pagina");
			inicial->size = tamanioPagina;
			recorrido += tamanioPagina - resto;
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 inicial->size);
			log_info(logger, "Recorrido  del alloc en esta pagina es %d ",
					 recorrido);
			memcpy(marco + obtenerResto(comienzo->nextAlloc),
				   alloc + size, (tamanioPagina - obtenerResto(comienzo->nextAlloc)));
		}
		else
		{
			log_info(logger, "requiere mas paginas");
			inicial->size = tamanioPagina;
			recorrido += tamanioPagina - resto;
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 inicial->size);
			log_info(logger, "Recorrido en esta pagina es %d ", recorrido);
		}
	}

	actualizarParametrosPAraAlgoritmos(inicial, proceso);

	while (limite != recorrido)
	{
		bool cantidadPresentesMemoria(void *elem)
		{
			Pagina *pag = elem;
			return pag->bitPresencia;
		};

		if (list_count_satisfying(proceso->tablaDePaginas, cantidadPresentesMemoria) == marcosPorCarpincho &&
			string_equals_ignore_case("FIJA", tipoAsignacion))
		{
			log_info(logger, "Aplicando sustitucion en FIJA");
			realizarSustitucionPagina(proceso, list_size(proceso->tablaDePaginas) - 1, cantidadPaginas(limite - recorrido));
		}
		else if (hayMarcosLibres() == false)
		{
			log_info(logger, "Aplicando sustitucion en Global");
			realizarSustitucionPagina(proceso, list_size(proceso->tablaDePaginas) - 1, cantidadPaginas(limite - recorrido));
		}

		if (tamanioPagina > (limite - recorrido))
		{
			log_info(logger, "La reserva restante cabe en esta pagina");
			Pagina *aux = crearPagina();
			if (string_equals_ignore_case("FIJA", tipoAsignacion))
			{
				aux->marco = obtenerMarcoLibreFijo(proceso->localLRUCLOCK);
			}
			else
			{
				aux->marco = obtenerMarcoLibre();
				log_info(logger, "el marco libre asignado es %d", aux->marco);
			}
			aux->size = limite - recorrido;
			aux->bitModificacion = true;
			inicial->bitUso = true;
			void *marcoAux = memoriaCompleta + (aux->marco * tamanioPagina);

			memcpy(marcoAux, alloc + recorrido, limite - recorrido);
			recorrido += limite - recorrido;
			list_add(proceso->tablaDePaginas, aux);
			controlarCreacionTLB(list_size(proceso->tablaDePaginas) - 1, aux->marco,
								 proceso->id_proceso);
			actualizarParametrosPAraAlgoritmos(aux, proceso);
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 aux->size);
			log_info(logger, "Recorrido en esta pagina es %d ", recorrido);
		}
		else
		{
			log_info(logger,
					 "Se requieren mas paginas , esta pagina  no es suficiente");
			Pagina *aux = crearPagina();
			if (string_equals_ignore_case("FIJA", tipoAsignacion))
			{
				aux->marco = obtenerMarcoLibreFijo(proceso->localLRUCLOCK);
			}
			else
			{
				aux->marco = obtenerMarcoLibre();
			}
			aux->size = tamanioPagina;

			recorrido += tamanioPagina;
			list_add(proceso->tablaDePaginas, aux);
			controlarCreacionTLB(list_size(proceso->tablaDePaginas) - 1, aux->marco,
								 proceso->id_proceso);
			if (string_equals_ignore_case("LRU", tipoSwap))
			{
				actualizadorVictimasLRU(aux, proceso->id_proceso, list_size(proceso->tablaDePaginas) - 1,
										proceso->localLRUCLOCK);
				aux->bitUso = true;
			}
			else
			{
				aux->bitUso = true;
			}
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 aux->size);
			log_info(logger, "Recorrido en esta pagina es %d ", recorrido);
		}
	}

	free(alloc);
	return dirLog = tamanioActual;
}

uint32_t obtenerMarcoLibreFijo(t_list *listaLRU)
{

	for (int i = 0; i < list_size(listaLRU); i++)
	{
		uint32_t *marco = list_get(listaLRU, i);
		Marco *analizar = list_get(listaDeMarcos, *marco);
		if (analizar->esLibre)
		{
			analizar->esLibre = false;
			return *marco;
		}
	}
	return -1;
}
bool hayMarcosLibres()
{
	for (int i = 0; i < list_size(listaDeMarcos); i++)
	{
		Marco *marco = list_get(listaDeMarcos, i);
		if (marco->esLibre)
		{
			log_info(logger, "Hay un marco libre");
			return true;
		}
	}
	log_info(logger, "No Hay un marco libre");
	return false;
}
uint32_t primeraAsignacion(int size, AdmProceso *proceso)
{

	HeapMetadata *inicial = malloc(sizeof(HeapMetadata));
	inicial->esLibre = false;
	inicial->nextAlloc = size + sizeof(HeapMetadata);
	inicial->prevAlloc = -1;
	uint32_t dirLog = 9;
	uint32_t desplazamiento = 0;
	uint32_t recorrido = 0;
	uint32_t limite = size + sizeof(HeapMetadata);
	log_info(logger, "El alloc previo de inicial es %d", inicial->prevAlloc);
	log_info(logger, "El alloc siguiente de inicial es %d", inicial->nextAlloc);

	void *alloc = crearAlloc(size, 0);
	uint32_t respuesta;

	Pagina *comienzo = crearPagina();

	if (string_equals_ignore_case("FIJA", tipoAsignacion))
	{
		comienzo->marco = obtenerMarcoLibreFijo(proceso->localLRUCLOCK);
		respuesta = enviarConsultaReserva(proceso->id_proceso, 0, (limite + sizeof(HeapMetadata)));
	}
	else
	{
		comienzo->marco = obtenerMarcoLibre();
		respuesta = enviarConsultaReserva(proceso->id_proceso, 0, (limite + sizeof(HeapMetadata)));
	}
	if (respuesta)
	{
		log_info(logger, "Paginas suficientes en Swamp");
	}
	else
	{
		dirLog = -1;
		return dirLog;
	}

	log_info(logger, "Se asigno un marco libre a la pagina %d", comienzo->marco);

	comienzo->size += sizeof(HeapMetadata);
	//Le copio el HeapMetadata inicial al marco libre asignado
	void *ptr_inicial = memoriaCompleta + (comienzo->marco * tamanioPagina);
	memcpy(ptr_inicial, inicial, sizeof(HeapMetadata));
	desplazamiento += sizeof(HeapMetadata);
	/*
	memcpy(memoriaCompleta + (comienzo->marco * tamanioPagina), inicial, sizeof(HeapMetadata));
	desplazamiento += sizeof(HeapMetadata) + (comienzo->marco * tamanioPagina);
*/
	if (limite < tamanioPagina - sizeof(HeapMetadata))
	{
		log_info(logger, "La reserva cave en la pagina");
		memcpy(ptr_inicial + desplazamiento, alloc + recorrido, limite);
		comienzo->size += limite;

		recorrido += limite;
		log_info(logger, "Tamanio ocupado en esta pagina es %d ",
				 comienzo->size);
		log_info(logger, "Recorrido en esta pagina es %d ", recorrido);
	}
	else
	{
		if ((tamanioPagina - obtenerResto(inicial->nextAlloc)) < 9)
		{
			log_info(logger, "El Heap quedara partido entre dos paginas");
			log_info(logger, "La reserva necesita otra pagina");
			recorrido += tamanioPagina - sizeof(HeapMetadata);
			comienzo->size += tamanioPagina - sizeof(HeapMetadata);
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 comienzo->size);
			log_info(logger, "Recorrido  del alloc en esta pagina es %d ",
					 recorrido);
			memcpy(ptr_inicial + obtenerResto(inicial->nextAlloc),
				   alloc + size, (tamanioPagina - obtenerResto(inicial->nextAlloc)));
		}
		else
		{
			log_info(logger, "La reserva necesita otra pagina");
			recorrido += tamanioPagina - sizeof(HeapMetadata);
			comienzo->size += tamanioPagina - sizeof(HeapMetadata);
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 comienzo->size);
			log_info(logger, "Recorrido  del alloc en esta pagina es %d ",
					 recorrido);
		}
	}
	log_info(logger, "El limite a detener es %d", limite);
	list_add(proceso->tablaDePaginas, comienzo);

	controlarCreacionTLB(list_size(proceso->tablaDePaginas) - 1, comienzo->marco,
						 proceso->id_proceso);
	actualizarParametrosPAraAlgoritmos(comienzo, proceso);

	while (limite != recorrido)
	{
		//Voy a seguir pidiendo paginas, en caso de que la siguiente pagina no sea suficiente
		bool cantidadPresentesMemoria(void *elem)
		{
			Pagina *pag = elem;
			return pag->bitPresencia;
		}

		if (list_count_satisfying(proceso->tablaDePaginas, cantidadPresentesMemoria) == marcosPorCarpincho &&
			string_equals_ignore_case("FIJA", tipoAsignacion))
		{
			log_info(logger, "Aplicando sustitucion en FIJA");
			realizarSustitucionPagina(proceso, list_size(proceso->tablaDePaginas) - 1, cantidadPaginas(limite - recorrido));
		}
		else if (hayMarcosLibres() == false)
		{
			log_info(logger, "Aplicando sustitucion en Global");
			realizarSustitucionPagina(proceso, list_size(proceso->tablaDePaginas) - 1, cantidadPaginas(limite - recorrido));
		}

		if (tamanioPagina > (limite - recorrido))
		{
			Pagina *aux = crearPagina();
			if (string_equals_ignore_case("FIJA", tipoAsignacion))
			{
				aux->marco = obtenerMarcoLibreFijo(proceso->localLRUCLOCK);
			}
			else
			{
				aux->marco = obtenerMarcoLibre();
			}
			aux->size = limite - recorrido;
			aux->bitModificacion = true;
			aux->bitUso = true;
			Marco *otro = list_get(listaDeMarcos, aux->marco);
			otro->esLibre = false;
			memcpy(otro->marco, alloc + recorrido, limite - recorrido);
			recorrido += limite - recorrido;
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 aux->size);
			log_info(logger, "Recorrido  del alloc en esta pagina es %d ",
					 recorrido);
			list_add(proceso->tablaDePaginas, aux);
			controlarCreacionTLB(list_size(proceso->tablaDePaginas) - 1, aux->marco,
								 proceso->id_proceso);

			actualizarParametrosPAraAlgoritmos(aux, proceso);
		}
		else
		{
			Pagina *aux = crearPagina();
			aux->size = tamanioPagina;
			if (string_equals_ignore_case("FIJA", tipoAsignacion))
			{
				aux->marco = obtenerMarcoLibreFijo(proceso->localLRUCLOCK);
			}
			else
			{
				aux->marco = obtenerMarcoLibre();
			}

			Marco *otro = list_get(listaDeMarcos, aux->marco);
			otro->esLibre = false;

			recorrido += tamanioPagina;
			log_info(logger, "Tamanio ocupado en esta pagina es %d ",
					 aux->size);
			log_info(logger, "Recorrido  del alloc en esta pagina es %d ",
					 recorrido);
			list_add(proceso->tablaDePaginas, aux);
			controlarCreacionTLB(list_size(proceso->tablaDePaginas) - 1, aux->marco,
								 proceso->id_proceso);
			if (string_equals_ignore_case("LRU", tipoSwap))
			{
				actualizadorVictimasLRU(aux, proceso->id_proceso, list_size(proceso->tablaDePaginas) - 1, proceso->localLRUCLOCK);
				comienzo->bitUso = true;
				comienzo->bitModificacion = false;
			}
			else
			{
				comienzo->bitUso = true;
				comienzo->bitModificacion = false;
			}
		}
	}
	free(alloc);
	return dirLog;
}

void *crearAlloc(uint32_t size, uint32_t dirLog)
{
	void *alloc = malloc(size + sizeof(HeapMetadata));
	HeapMetadata * final = malloc(sizeof(HeapMetadata));
	final->esLibre = true;
	final->nextAlloc = -1;
	final->prevAlloc = dirLog;
	memcpy(alloc + size, final, sizeof(HeapMetadata));
	return alloc;
}

//--------------------------Funciones del memRead-------------------------

void RealizarLecturaPaginas(void *buffer, int socket)
{
	Memread *recibido2 = deserializarMemread(buffer);

	log_info(logger, "La pid recibida a leer es %d ", recibido2->pid);

	log_info(logger, "La direccion logica de lectura recibido es %d ",
			 recibido2->dirLog);

	log_info(logger, "El size a leer es %d ", recibido2->size);

	uint32_t tamanioContenido = 0;
	uint32_t respuesta = 0;
	mostrarListaTLBs();
	AdmProceso *proceso = buscarCarpincho(recibido2->pid);

	if (proceso->tamanio > recibido2->dirLog)
	{
		
		void *contenido = verificarPaginasAllocConTLBParaLectura(recibido2->pid,
																 recibido2->dirLog, recibido2->size, &tamanioContenido);

		if (contenido == NULL)
		{

			log_info(logger, "No se encontro en la TLB accediendo a paginas");
			AdmProceso *buscado = buscarCarpincho(recibido2->pid);

			void *contenidoEx = retornarContenidoAllocLectura(recibido2->dirLog,
															  buscado->tablaDePaginas, recibido2->pid, recibido2->size);
			log_info(logger,"Contenido obtenido %s",contenidoEx);
			mostrarListaTLBs();
			mostrarPaginasTotales(buscado->tablaDePaginas, buscado->id_proceso);
			uint32_t tamanioEnvio = 0;

			void *aEnviar = serializarContenidoRespuesta(contenidoEx, recibido2->size, respuesta, &tamanioEnvio);
            log_info(logger,"El tamanio de Envio es %d",tamanioEnvio);
			send(socket, aEnviar, tamanioEnvio, 0);

			log_info(logger, "Realizo el envio del contenido");
			free(aEnviar);
			free(contenidoEx);
		}
		else
		{
			log_info(logger, "Se encontro en la TLB accediendo al contenido");
			//Borrar proceso sol odemostracions
			AdmProceso *buscado = buscarCarpincho(recibido2->pid);

			log_info(logger,"Contenido obtenido %s",contenido);
			mostrarListaTLBs();
			mostrarPaginasTotales(buscado->tablaDePaginas, buscado->id_proceso);
			uint32_t tamanioEnvio = 0;
			log_info(logger, "contenido comprobacion: %s", contenido);

			void *aEnviar = serializarContenidoRespuesta(contenido, recibido2->size, respuesta, &tamanioEnvio);

			send(socket, aEnviar, tamanioEnvio, 0);
			log_info(logger, "Realizao el envio del contenido");
			free(aEnviar);
			free(contenido);
		}
	}
	else
	{
		log_info(logger, "No se encontro el contenido esta vacio");
		uint32_t tamanioEnvio = 0;

		void *contenidoVacio = malloc(recibido2->size);
		respuesta = -1;
		void *aEnviar = serializarContenidoRespuesta(contenidoVacio, recibido2->size, respuesta, &tamanioEnvio);

		send(socket, aEnviar, tamanioEnvio, 0);

		log_info(logger, "Realizao el envio del contenido");
		free(aEnviar);
		free(contenidoVacio);
	}
}

void *retornarContenidoAllocLectura(uint32_t dirLog, t_list *paginas,
									uint32_t id, uint32_t size)
{
    void *contenido=malloc(size);
	AdmProceso* proceso=buscarCarpincho(id);
	Pagina *inicial = list_get(paginas, obtenerDeterminadaPagina(dirLog));
	uint32_t desplazamiento = 0;
	//Compruevo la precencia de la primera pagina
	if (inicial->bitPresencia)
	{
		log_info(logger, "Pagina inicial presente");
	}
	else
	{
		log_info(logger, "Se recupera la pagina inicial");
		recuperacionPaginaFaltante(id, obtenerDeterminadaPagina(dirLog));
	}
	//Veridico si el HeapMetadata esta entre dos pagina o no y retorno el HEAP.
	log_info(logger,
			 "comenzando verificacion de marcos en memoria para escritura");

	HeapMetadata *inicio;
	void *ptrAlloc = memoriaCompleta + (inicial->marco * tamanioPagina) + obtenerResto(dirLog);

	uint32_t tamanioContenido = inicio->nextAlloc - dirLog;

	

	uint32_t tamanioAlloc = inicio->nextAlloc - dirLog;
	uint32_t recorrido = 0;
	//En caso de que se quiera escribir contenido mayor al tamaño que puede contener

	if ((tamanioPagina - obtenerResto(dirLog)) > size)
	{

		log_info(logger, "Lectura en la misma pagina");

		memcpy(contenido, ptrAlloc, size);
		return contenido;
	}
	else
	{
		memcpy(contenido, ptrAlloc,
			   tamanioPagina - obtenerResto(dirLog));
		recorrido += tamanioPagina - obtenerResto(dirLog);
	}
	uint16_t i = obtenerDeterminadaPagina(dirLog) + 1;

	while (recorrido != size)
	{
		TLB *encontrada = devolberTLBconPagina(i, id);
		if (encontrada == NULL)
		{
			log_info(logger, "La TLB no esta presente en la lista");
			recuperacionPaginaFaltante(id, i);
			//Se recuperaron las paginas, se realizaron accesos a las paginas victima y solicitada para
			//Modificar sus datos

			log_info(logger,
					 "La pagina %d fue devuelta con exito de Swamp", i);
			TLBmissGlobal++;
			proceso->miss++;
			Pagina* devuelta=list_get(paginas,i);
			log_info(logger, "Lectura en la pagina siguiente");
			if (tamanioPagina < (size - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((devuelta->marco * tamanioPagina));
				memcpy(contenido, ptrAlloc, tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((devuelta->marco * tamanioPagina));
				memcpy(contenido, ptrAlloc,
					   (size - recorrido));
				recorrido += tamanioContenido - recorrido;

				mostrarListaTLBs();

				return contenido;
			}
		}
		else
		{
			TLBhitGlobal++;
			proceso->hit++;
			log_info(logger, "Lectura en la pagina siguiente");
			if (tamanioPagina < (size - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(contenido, ptrAlloc, tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(contenido, ptrAlloc,
					   (size - recorrido));
				recorrido += tamanioContenido - recorrido;

				mostrarListaTLBs();

				return contenido;
			}
		}

		i++;
	}


return contenido;
}

//-----------------------------Funciones del memWrite--------------------------------

void realizarEscrituraMemoria(void *buffer, int socket)
{
	Memwrite *recibido = deserializarMemwrite(buffer);
	log_info(logger, "La pid recibida es %d ", recibido->pid);

	log_info(logger, "La direccion logica recibido es %d ", recibido->dirLog);

	log_info(logger, "El tamanio del contenido es %d ", recibido->tamanioCont);

	log_info(logger, "El contenido a guardar es %s ", recibido->contenido);

	uint32_t respuesta = -7;

	if (escribirAllocConListaTLB(recibido->pid, recibido->dirLog,
								 recibido->tamanioCont, recibido->contenido))
	{
		//-----------------------------Para confirmar Datos--------------------
		AdmProceso *buscado = buscarCarpincho(recibido->pid);
		mostrarListaTLBs();
		mostrarPaginasTotales(buscado->tablaDePaginas, buscado->id_proceso);
		//---------------------------------------------------
		uint32_t respuesta = 0;
		send(socket, &(respuesta), sizeof(uint32_t), 0);
		log_info(logger, "Escritura en el alloc %d realizada con exito", recibido->dirLog);
	}
	else
	{

		AdmProceso *buscado = buscarCarpincho(recibido->pid);

		if (verificarPaginasPresentesAlloc(recibido->dirLog,
										   buscado->tablaDePaginas, recibido->pid, recibido->tamanioCont,
										   recibido->contenido))
		{
			respuesta = 0;
			mostrarListaTLBs();
			mostrarPaginasTotales(buscado->tablaDePaginas, buscado->id_proceso);
			log_info(logger, "Escritura en el alloc %d realizada con exito", recibido->dirLog);
			send(socket, &(respuesta), sizeof(uint32_t), 0);
		}
		else
		{

			send(socket, &respuesta, sizeof(uint32_t), 0);
		}
	}
}

bool verificarPaginasPresentesAlloc(uint32_t dirLog, t_list *paginas,
									uint32_t id, uint32_t tamanioContenido, void *contenido)
{
	AdmProceso* proceso=buscarCarpincho(id);
	void *ptrMarco;

	Pagina *inicial = list_get(paginas, obtenerDeterminadaPagina(dirLog));
	//Compruevo la precencia de la primera pagina
	if (inicial->bitPresencia)
	{
		log_info(logger, "Pagina inicial presente");
		ptrMarco = memoriaCompleta + ((inicial->marco * tamanioPagina)+ (obtenerResto(dirLog)));
	}
	else
	{
		log_info(logger, "Se recupera la pagina inicial");
		recuperacionPaginaFaltante(id, obtenerDeterminadaPagina(dirLog));
		ptrMarco = memoriaCompleta + (inicial->marco * tamanioPagina);
	}
	//Veridico si el HeapMetadata esta entre dos pagina o no y retorno el HEAP.
	log_info(logger,
			 "comenzando verificacion de marcos en memoria para escritura");
	HeapMetadata *inicio;

	if (obtenerResto(dirLog) < 9)
	{
		log_info(logger, "Partido entre dos");
		inicio = entreDosPaginas(dirLog - sizeof(HeapMetadata), proceso);
	}
	else
	{
		log_info(logger, "HeapMetadata normal");
		inicio = ptrMarco - sizeof(HeapMetadata);
	}

	uint32_t tamanioAlloc = inicio->nextAlloc - dirLog;
	uint32_t recorrido = 0;
	log_info(logger, "HeapMetadata prev inicio %d", inicio->prevAlloc);
	log_info(logger, "HeapMetadata next inicio %d", inicio->nextAlloc);
	//Veridico si el alloc puede contener la escritura en caso negativo retorno false

	if (tamanioAlloc < tamanioContenido)
	{
		log_info(logger, "ERROR DE ESCRITURA: contenido demasiado grande");
		return false;
	}
	

	//En caso de que se quiera escribir contenido mayor al tamaño que puede contener
   
	if ((tamanioPagina - obtenerResto(dirLog)) > tamanioAlloc)
	{

		log_info(logger, "Lectura en la misma pagina");

		memcpy(ptrMarco, contenido, tamanioContenido);
		return contenido;
	}
	else
	{
		memcpy(ptrMarco, contenido,
			   tamanioPagina - obtenerResto(dirLog));
		recorrido += tamanioPagina - obtenerResto(dirLog);
	}
	uint16_t i = obtenerDeterminadaPagina(dirLog) + 1;

	while (recorrido != tamanioContenido)
	{
		void* ptrAlloc;
		TLB *encontrada = devolberTLBconPagina(i, id);
		if (encontrada == NULL)
		{
			log_info(logger, "La TLB no esta presente en la lista");
			recuperacionPaginaFaltante(id, i);
			//Se recuperaron las paginas, se realizaron accesos a las paginas victima y solicitada para
			//Modificar sus datos
			Pagina *devuelta = list_get(paginas, i);
			log_info(logger,
					 "La pagina %d fue devuelta con exito de Swamp", i);
			TLBmissGlobal++;
			proceso->miss++;
			if (tamanioPagina < (tamanioContenido - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((devuelta->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido, tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((devuelta->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido,
					   (tamanioContenido - recorrido));
				recorrido += tamanioContenido - recorrido;

				mostrarListaTLBs();

				return true;
			}
		}
		else
		{
			TLBhitGlobal++;
			proceso->hit++;
			if (tamanioPagina < (tamanioContenido - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido, tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido,
					   (tamanioContenido - recorrido));
				recorrido += tamanioContenido - recorrido;

				mostrarListaTLBs();

				return true;
			}
		}
		i++;
		log_info(logger, "Escritura en la pagina siguiente");
	}
	return true;
}

void *juntarContenidoMemoria(t_list *paginas, uint32_t tamanioTotal)
{
	int cantidadPaginas = list_size(paginas);
	void *contenidoMem = malloc(cantidadPaginas * tamanioPagina);
	int desplazamiento = 0;
	for (int i = 0; i < cantidadPaginas; i++)
	{
		void *marco = ptr_marco(i, paginas);
		memcpy(contenidoMem + desplazamiento, marco, tamanioPagina);
		desplazamiento += tamanioPagina;
	}
	log_info(logger, "Tamanio Total de los marcos juntos %d", desplazamiento);
	return contenidoMem;
}

//---------------------------Funciones para memfree----------------------------

void realizarMemFree(void *buffer, int socket)
{
	MemFree *recibido = deserializarMemFree(buffer);
	log_info(logger, "La direccion el alloc tiene como inicio la direccion logica %d",
			 recibido->dirLog);
	AdmProceso *proceso = buscarCarpincho(recibido->pid);
	log_info(logger, " cantidad de paginas actual %d",
			 list_size(proceso->tablaDePaginas));

	uint32_t respuesta = memfree(recibido->dirLog, proceso);

	log_info(logger, "Cantidad de paginas luego del memfree %d",
			 list_size(proceso->tablaDePaginas));
	mostrarPaginasTotales(proceso->tablaDePaginas, proceso->id_proceso);
	log_info(logger, "El estatus a enviar a Kernel es %d", respuesta);

	//send(socket,&respuesta,sizeof(uint32_t),0);
}

void *ptr_marco(uint32_t numeroPag, t_list *paginas)
{
	Pagina *buscada = list_get(paginas, numeroPag);
	Marco *encontrado = list_get(listaDeMarcos, buscada->marco);
	return encontrado->marco;
}

uint32_t memfree(uint32_t dirLog, AdmProceso *proceso)
{
	Pagina *paginaPrincipal = list_get(proceso->tablaDePaginas, obtenerDeterminadaPagina(dirLog));

	if (paginaPrincipal->bitPresencia == false)
	{
		recuperacionPaginaFaltante(proceso->id_proceso, obtenerDeterminadaPagina(dirLog));
		mostrarPaginasTotales(proceso->tablaDePaginas, 1);
	}

	HeapMetadata *actual = memoriaCompleta + ((paginaPrincipal->marco * tamanioPagina) + (obtenerResto(dirLog) - sizeof(HeapMetadata)));

	log_info(logger, "HeapMetadata Actual Previo  %d", actual->prevAlloc);
	log_info(logger, "HeapMetadata Actual Siguiente %d", actual->nextAlloc);
	Pagina *anterior;
	Pagina *siguiente;

	if (obtenerDeterminadaPagina(dirLog) == obtenerDeterminadaPagina(actual->nextAlloc))
	{
		log_info(logger, "El Heap derecho esta en la misma pagina");
		siguiente = paginaPrincipal;
	}
	else
	{
		siguiente = list_get(proceso->tablaDePaginas, obtenerDeterminadaPagina(actual->nextAlloc));
		if (siguiente->bitPresencia == false)
		{
			log_info(logger, "La pagina derecha no esta presente en memoria");
			recuperacionPaginaFaltante(proceso->id_proceso, obtenerDeterminadaPagina(actual->nextAlloc));
			mostrarPaginasTotales(proceso->tablaDePaginas, 1);
		}
		log_info(logger, "La pagina siguiente esta en memoria");
	}
	log_info(logger, "Comenzando el analisis del alloc anterior");

	HeapMetadata *der = memoriaCompleta + ((siguiente->marco * tamanioPagina) + obtenerResto(actual->nextAlloc));

	log_info(logger, "HeapMetadata Derecha Previo  %d",
			 der->prevAlloc);
	log_info(logger, "HeapMetadata Derecha Siguiente %d",
			 der->nextAlloc);
	//============================================Si queda un unico alloc====================
	if (actual->prevAlloc == -1 && der->nextAlloc == -1)
	{
		log_info(logger, "Ultimo alloc proceder a borrar");
		uint32_t cantidadAEliminar = list_size(proceso->tablaDePaginas);
		log_info(logger, "Cantidad de pagians a eliminar %d", cantidadAEliminar);
		uint32_t desplazamientoPagina = 0;
		for (int i = 0; i < cantidadAEliminar; i++)
		{

			Pagina *aQuitar = list_remove(proceso->tablaDePaginas, 0);
			Marco *aliberar = list_get(listaDeMarcos, aQuitar->marco);
			aliberar->esLibre = true;
			free(aQuitar);
			bool removerTLBPorMarco(void *elem)
			{
				TLB *sacar = elem;
				return sacar->marco == aQuitar->marco;
			}
			TLB *eliminar = list_remove_by_condition(registroTLB, removerTLBPorMarco);

			if (eliminar != NULL)
			{
				free(eliminar);
			}
			verificarNotificacionLiberacionMarco(proceso->id_proceso, desplazamientoPagina);
			desplazamientoPagina++;
		}
		return 0;
	}
	//====================================================================================
	HeapMetadata *izq;
	if (actual->prevAlloc != -1)
	{
		if (obtenerDeterminadaPagina(dirLog) == obtenerDeterminadaPagina(actual->prevAlloc))
		{
			log_info(logger, "El Heap izquierdo esta en la misma pagina");
			anterior = paginaPrincipal;
		}
		else
		{
			anterior = list_get(proceso->tablaDePaginas, obtenerDeterminadaPagina(actual->prevAlloc));
			if (anterior->bitPresencia == false)
			{
				log_info(logger, "La pagina izquierda no esta presente en memoria");
				recuperacionPaginaFaltante(proceso->id_proceso,
										   obtenerDeterminadaPagina(actual->prevAlloc));
				mostrarPaginasTotales(proceso->tablaDePaginas, 1);
			}
		}
		izq = memoriaCompleta + ((anterior->marco * tamanioPagina) +
								 obtenerResto(actual->prevAlloc));
		log_info(logger, "HeapMetadata Izquierdo Previo  %d",
				 izq->prevAlloc);
		log_info(logger, "HeapMetadata Izquierdo Siguiente %d",
				 izq->nextAlloc);

		if (der->nextAlloc == -1 && izq->prevAlloc == -1 && list_size(proceso->tablaDePaginas) > 3)
		{

			log_info(logger, "Ultimo alloc proceder a borrar");
			uint32_t cantidadAEliminar = list_size(proceso->tablaDePaginas);
			log_info(logger, "Cantidad de pagians a eliminar %d", cantidadAEliminar);
			uint32_t paginaDesplazamiento = cantidadAEliminar;
			for (int i = 0; i < cantidadAEliminar; i++)
			{

				Pagina *aQuitar = list_remove(proceso->tablaDePaginas, 0);
				free(aQuitar);
				bool removerTLBPorMarco(void *elem)
				{
					TLB *sacar = elem;
					return sacar->marco == aQuitar->marco;
				}
				TLB *eliminar = list_remove_by_condition(registroTLB, removerTLBPorMarco);
				if (eliminar != NULL)
				{
					free(eliminar);
				}

				verificarNotificacionLiberacionMarco(proceso->id_proceso, paginaDesplazamiento);
				paginaDesplazamiento--;
			}
			return 0;
		}
		else if (izq->esLibre)
		{
			log_info(logger, "El heap izquierdo esta libre procediendo a realizar cambios");
			izq->nextAlloc = actual->nextAlloc;
			der->prevAlloc = actual->prevAlloc;
			actual->esLibre = true;
		}
	}
	else
	{
		izq = actual;
		paginaPrincipal->size = 0;
		actual->esLibre = true;
	}

	log_info(logger, "Comenzando el analisis del alloc siguiente");

	if (der->nextAlloc == -1 && izq->esLibre)
	{
		log_info(logger, "El heapMetadata anterior se convierte en el final primera opcion");
		actual->nextAlloc = -1;
		izq->nextAlloc = der->nextAlloc;
		actual->esLibre = true;
	}
	else if (der->nextAlloc == -1)
	{
		log_info(logger, "El heapMetadata anterior se convierte en el final");
		actual->nextAlloc = -1;
		actual->esLibre = true;
	}
	else if (der->esLibre && der->nextAlloc != -1)
	{
		log_info(logger, "El derecho esta libre apuntando mas atras de %d a %d ",
				 der->prevAlloc, actual->prevAlloc);
		log_info(logger, "Apuntando al HeapSiguiente, consolidando Heaps,Apuntando siguiente de %d a %d",
				 actual->nextAlloc, der->nextAlloc);
		actual->nextAlloc = der->nextAlloc;
		actual->esLibre = true;
		Pagina *siguienteEx = list_get(proceso->tablaDePaginas, obtenerDeterminadaPagina(der->nextAlloc));
		HeapMetadata *siguienEx = memoriaCompleta + (tamanioPagina * siguienteEx->marco) +
								  obtenerResto(der->nextAlloc);
		siguienEx->prevAlloc = der->prevAlloc;
	}
	else
	{

		actual->esLibre = true;
	}

	paginaPrincipal->bitModificacion = true;

	//recorrerHeaps(proceso->tablaDePaginas);

	if (izq->esLibre && izq->nextAlloc == -1)
	{
		log_info(logger, "Borrando ultimas paginas sobrantes");
		liberarMarcoLibre(actual, proceso, actual->prevAlloc);
	}
	else if (der->prevAlloc == -1)
	{
		log_info(logger, "Liberando marcos del comienzo");
		liberarMarcoLibre(der, proceso, der->nextAlloc);
	}
	else
	{
		log_info(logger, "Liberacion normal");
		liberarMarcoLibre(actual, proceso, dirLog);
	}
	if (izq->esLibre && obtenerDeterminadaPagina(dirLog) != obtenerDeterminadaPagina(actual->nextAlloc))
	{

		modificarPaginaYliberarMarco(proceso, obtenerDeterminadaPagina(dirLog));
		verificarNotificacionLiberacionMarco(proceso->id_proceso, obtenerDeterminadaPagina(dirLog));
	}

	//Comprobacion borrar

	log_info(logger, "HeapMetadata izq Previo  %d",
			 izq->prevAlloc);
	log_info(logger, "HeapMetadata izq Siguiente %d",
			 izq->nextAlloc);

	log_info(logger, "HeapMetadata derecho Previo  %d",
			 der->prevAlloc);
	log_info(logger, "HeapMetadata derecho Siguiente %d",
			 der->nextAlloc);
	mostrarPaginasTotales(proceso->tablaDePaginas, proceso->id_proceso);
	return 0;
}

void recorrerHeaps(t_list *paginas)
{

	log_info(logger, "======================================================================");
	log_info(logger, "Verificando integridad de Heaps");

	uint32_t final = 0;
	while (final != -1)
	{
		Pagina *pagina = list_get(paginas, obtenerDeterminadaPagina(final));

		//trayendo paginas
		if (pagina->bitPresencia == false)
		{
			recuperacionPaginaFaltante(1, obtenerDeterminadaPagina(final));
		}

		HeapMetadata *heap = memoriaCompleta + (pagina->marco * tamanioPagina) + obtenerResto(final);
		log_info(logger, "HeapMetadata actualizado Previo  %d",
				 heap->prevAlloc);
		log_info(logger, "HeapMetadata actualizado Siguiente %d",
				 heap->nextAlloc);
		log_info(logger, "Es libre True= 1 / False=0  Resultado=%d", heap->esLibre);

		final = heap->nextAlloc;
	}
	log_info(logger, "======================================================================");
}

void liberarMarcoLibre(HeapMetadata *actual, AdmProceso *proceso, uint32_t dirLog)
{

	if (actual->nextAlloc == -1)
	{
		log_info(logger, "Eliminando paginas sobrantes del final");
		uint32_t cantidadAborrar = (list_size(proceso->tablaDePaginas) - 1) - obtenerDeterminadaPagina(dirLog);
		log_info(logger, "Cantidad de paginas a Borrar %d", cantidadAborrar);
		uint32_t tamanioLista = list_size(proceso->tablaDePaginas);
		for (int i = 0; i < cantidadAborrar; i++)
		{
			Pagina * final = list_remove(proceso->tablaDePaginas, tamanioLista - 1);
			Marco *marco = list_get(listaDeMarcos, final->marco);
			marco->esLibre = true;
			verificarNotificacionLiberacionMarco(proceso->id_proceso, (uint16_t)tamanioLista - 1);
			tamanioLista--;

			free(final);
		}
	}
	else
	{
		uint32_t paginasLiberadasSiguiente = obtenerDeterminadaPagina(actual->nextAlloc) -
											 obtenerDeterminadaPagina(dirLog);

		uint32_t paginasLiberadasanterior = obtenerDeterminadaPagina(actual->nextAlloc) -
											obtenerDeterminadaPagina(actual->prevAlloc);

		if (paginasLiberadasSiguiente > 0)
		{
			log_info(logger, "PAginas Quese pueden liberar hacia derecha %d ",
					 paginasLiberadasSiguiente);
			for (uint16_t i = obtenerDeterminadaPagina(dirLog) + 1;
				 i < obtenerDeterminadaPagina(actual->nextAlloc); i++)
			{

				modificarPaginaYliberarMarco(proceso, i);
			}
		}
		if (paginasLiberadasanterior > 0)
		{

			log_info(logger, "PAginas Quese pueden liberar hacia izquierda %d ",
					 paginasLiberadasanterior);
			if (actual->prevAlloc == 0)
			{
				log_info(logger, "Liberando paginas luego de la primera pagina");
				for (uint16_t j = 1;
					 j < obtenerDeterminadaPagina(actual->nextAlloc); j++)
				{
					modificarPaginaYliberarMarco(proceso, j);
				}
			}
		}
	}
}
void modificarPaginaYliberarMarco(AdmProceso *proceso, uint16_t pagina)
{
	log_info(logger, "Modificando Pagina,y elimnando tlb");
	Pagina *aModificar = list_get(proceso->tablaDePaginas, pagina);
	if (aModificar->marco != -1)
	{
		Marco *aLiberar = list_get(listaDeMarcos, aModificar->marco);
		aLiberar->esLibre = true;
		bool removerTLBPorMarco(void *elem)
		{
			TLB *sacar = elem;
			return sacar->marco == aModificar->marco;
		}
		TLB *eliminar = list_remove_by_condition(registroTLB, removerTLBPorMarco);

		free(eliminar);
		aModificar->marco = -1;
		aModificar->size = 0;
	}
}

void verificarNotificacionLiberacionMarco(uint32_t pid, uint16_t pagina)
{
	if (string_equals_ignore_case("GLOBAL", tipoAsignacion))
	{
		log_info(logger, "Desasignando marco en swamp");
		realizarEnvioLiberacioMarco(pid, pagina);
	}
}
//------------------------------------Funciones para el SWAP de paginas----------------

void *juntarContenidoParaSwampAlloc(t_list *paginas, uint32_t alloc,
									uint32_t *tamanioAlloc, uint16_t *pagina)
{
	int resto = obtenerResto(alloc);
	log_info(logger, "Resto %d ", resto);
	log_info(logger, "Direccion Logica %d ", alloc);
	log_info(logger, "Pagina %d ", obtenerDeterminadaPagina(alloc));

	uint32_t contador = obtenerDeterminadaPagina(alloc);
	HeapMetadata *inicial;
	void *contenido;
	int desplazamiento = 0;
	if (resto < 9)
	{
		log_info(logger, "HeapMetadata partido");
		void *marcoPrimero = ptr_marco(contador - 1, paginas);

		uint32_t cantidadPaginasEntreAllocs = 2 + obtenerDeterminadaPagina(inicial->nextAlloc) - contador;
		log_info(logger, "Cantidad de paginas a copiar y enviar %d",
				 cantidadPaginasEntreAllocs);
		contenido = malloc((cantidadPaginasEntreAllocs + 1) * tamanioPagina);
		memcpy(contenido, marcoPrimero, tamanioPagina);
		desplazamiento += tamanioPagina;
		(*tamanioAlloc) = cantidadPaginasEntreAllocs * tamanioPagina;
		(*pagina) = contador - 1;
	}
	else
	{
		log_info(logger, "HeapMetadata normal");
		inicial = ptr_marco(obtenerDeterminadaPagina(alloc), paginas) + (resto - sizeof(HeapMetadata));
		uint32_t cantidadPaginasEntreAllocs = 1 + obtenerDeterminadaPagina(inicial->nextAlloc) - contador;

		log_info(logger, "Cantidad de paginas a copiar y enviar %d",
				 cantidadPaginasEntreAllocs);

		contenido = malloc(cantidadPaginasEntreAllocs * tamanioPagina);
		(*tamanioAlloc) = cantidadPaginasEntreAllocs * tamanioPagina;
		(*pagina) = contador;
	}

	log_info(logger, "HeapMetadata prev %d ", inicial->prevAlloc);

	log_info(logger, "HeapMetadata sig %d ", inicial->nextAlloc);

	log_info(logger, "Pagina inicio %d ", contador);

	while (contador < obtenerDeterminadaPagina(inicial->nextAlloc) + 1)
	{
		void *aux = ptr_marco(contador, paginas);
		memcpy(contenido + desplazamiento, aux, tamanioPagina);
		desplazamiento += tamanioPagina;
		contador++;
	}

	log_info(logger, "Tamanio del desplazamiento total del envio %d",
			 desplazamiento);
	/*
	 if (obtenerDeterminadaPagina(alloc)== obtenerDeterminadaPagina(inicial->nextAlloc)) {


	 return contenido;
	 } else {
	 uint32_t limite = tamanioAlloc;
	 int i = obtenerDeterminadaPagina(alloc);
	 void*marco = ptr_marco(obtenerDeterminadaPagina(alloc), paginas);
	 memcpy(contenido + desplazamiento,
	 marco + (resto - sizeof(HeapMetadata)),
	 tamanioPagina - (resto - sizeof(HeapMetadata)));

	 limite -= tamanioPagina - (resto - sizeof(HeapMetadata));
	 desplazamiento += tamanioPagina - (resto - sizeof(HeapMetadata));
	 i++;
	 while (limite != 0) {
	 Pagina *buscada = list_get(paginas, i);
	 Marco* encontrado = list_get(listaDeMarcos, buscada->marco);
	 if (tamanioPagina > limite) {
	 memcpy(contenido + desplazamiento, encontrado->marco, limite);
	 limite -= limite;

	 } else {

	 memcpy(contenido + desplazamiento, encontrado->marco,
	 tamanioPagina);
	 limite -= tamanioPagina;
	 }
	 i++;

	 }
	 */
	return contenido;
}

int encontrarUnaVictimaLocalLRU(AdmProceso *proceso)
{

	int *elegido;
	mostrarListaCLOCKLRU(proceso->localLRUCLOCK);
	elegido = list_remove(proceso->localLRUCLOCK, 0);
	log_info(logger, "Marco Victima %d", *elegido);

	return elegido;

	/*
	 Pagina* victima = list_get(proceso->tablaDePaginas, elegido);
	 victima->bitPresencia = false;
	 mostrarListaLRU(proceso->localLRU);
	 return victima->marco;
	 */
}

int encontrarUnaVictimaLocalClockM(AdmProceso *proceso)
{
	bool hayVictima = false;
	uint32_t victima = -1;
	uint32_t opcion = 2;
	uint32_t flagClock = proceso->referencioClock;
	uint32_t posPagina = 0;
	log_info(logger, "numero Flag %d y referencia lista %d",
			 flagClock, proceso->referencioClock);

	while (hayVictima == false)
	{
		mostrarListaCLOCKLRU(proceso->localLRUCLOCK);

		for (; proceso->referencioClock < list_size(proceso->localLRUCLOCK);
			 proceso->referencioClock++)
		{
			log_info(logger, "Comenzando analisis de la posicion %d", proceso->referencioClock);
			Pagina *aEvaluar = buscarPaginaRelacionadaMarco(proceso, proceso->referencioClock, &posPagina);
			log_info(logger, "Datos Pagina: BitM %d BitU %d BitP %d,Marco %d Pagina %d", aEvaluar->bitModificacion,
					 aEvaluar->bitUso, aEvaluar->bitPresencia, aEvaluar->marco, posPagina);
			if (flagClock == proceso->referencioClock)
			{
				if (opcion == 1)
				{
					opcion = 2;
				}
				else
				{

					opcion = 1;
				}
			}
			switch (opcion)
			{
			case 1:
				log_info(logger, "Primer analisis CLOCK");
				if (aEvaluar->bitPresencia)
				{
					if (aEvaluar->bitUso == 0 && aEvaluar->bitModificacion == 0)
					{

						aEvaluar->bitPresencia = false;
						log_info(logger, "La posicion en la lista Clock es %d", proceso->referencioClock);
						proceso->referencioClock++;
						proceso->referencioClock = controlPosicionClock(proceso->referencioClock, list_size(proceso->localLRUCLOCK));
						log_info(logger, "Nuevo comienzo referencia %d", proceso->referencioClock);

						return aEvaluar->marco;
						hayVictima = true;
					}
				}
				break;
			case 2:
				log_info(logger, "Segundo analisis seteando bit uso en 0 y buscando MOD");
				if (aEvaluar->bitPresencia)
				{
					if (aEvaluar->bitUso == 0 && aEvaluar->bitModificacion == 1)
					{
						aEvaluar->bitModificacion = 0;
						void *aEnviar = malloc(tamanioPagina);
						memcpy(aEnviar,
							   memoriaCompleta + (aEvaluar->marco * tamanioPagina),
							   tamanioPagina);
						log_info(logger, "Se enviara a Swamp la victima Pagina %d ID %d",
								 proceso->referencioClock, proceso->id_proceso);
						enviarVictimaPageFault(proceso->id_proceso, posPagina, aEnviar);

						log_info(logger, "La posicion en la lista Clock es %d", proceso->referencioClock);
						proceso->referencioClock++;
						proceso->referencioClock = controlPosicionClock(proceso->referencioClock, list_size(proceso->localLRUCLOCK));
						log_info(logger, "Nuevo comienzo referencia %d", proceso->referencioClock);

						return aEvaluar->marco;
					}
					else
					{
						aEvaluar->bitUso = 0;
					}
				}
				break;
			default:
				break;
			}
		}
		mostrarPaginasTotales(proceso->tablaDePaginas, proceso->id_proceso);
		proceso->referencioClock = 0;
	}

	return victima;
}
Pagina *buscarPaginaRelacionadaMarco(AdmProceso *proceso, uint32_t posicion, uint32_t *paginaPos)
{
	uint32_t *marcoLista = list_get(proceso->localLRUCLOCK, posicion);
	for (uint32_t i = 0; i < list_size(proceso->tablaDePaginas); i++)
	{
		Pagina *pagina = list_get(proceso->tablaDePaginas, i);
		if (pagina->marco == (*marcoLista))
		{
			(*paginaPos) = i;
			return pagina;
		}
	}
	return NULL;
}
uint32_t controlPosicionClock(uint32_t posicion, uint32_t limite)
{
	if (posicion == limite)
	{
		log_info(logger, "Seteando a 0");
		return 0;
	}
	else
	{
		log_info(logger, "Seteo OK");
		return posicion;
	}
}
referenciaGlobalClockLRU *encontrarUnaVictimaGlobalClockM()
{
	bool hayVictima = false;

	referenciaGlobalClockLRU *encontrada;
	uint32_t opcion = 2;
	uint32_t flagClock = posicionClock;
	while (hayVictima == false)
	{

		mostrarListaCLOCKLRU(globalLRUCLOCK);
		uint32_t tamanio = list_size(globalLRUCLOCK);

		log_info(logger, "La flag clock esta seteado en %d", flagClock);
		log_info(logger, "tamanio de la lista CLock %d", tamanio);
		for (; posicionClock < tamanio; posicionClock++)
		{
			encontrada = list_get(globalLRUCLOCK, posicionClock);
			log_info(logger, "Encontrada referencia global Marco %d Pagina %d ID %d",
					 encontrada->marco, encontrada->pagina, encontrada->id_carpincho);
			AdmProceso *proceso = buscarCarpincho(encontrada->id_carpincho);
			Pagina *pagina = list_get(proceso->tablaDePaginas, encontrada->pagina);
			log_info(logger, "Datos Pagina: BitM %d BitU %d BitP %d",
					 pagina->bitModificacion, pagina->bitUso, pagina->bitPresencia);
			if (flagClock == posicionClock)
			{
				if (opcion == 1)
				{
					opcion = 2;
				}
				else
				{

					opcion = 1;
				}
			}
			switch (opcion)
			{
			case 1:
				log_info(logger, "Primer analisis CLOCK");
				if (pagina->bitPresencia)
				{
					if (pagina->bitUso == 0 && pagina->bitModificacion == 0)
					{

						pagina->bitPresencia = false;
						log_info(logger, "La posicion en la lista Clock es %d", posicionClock);
						posicionClock++;
						posicionClock = controlPosicionClock(posicionClock, tamanio);
						return encontrada;
						hayVictima = true;
					}
				}
				break;
			case 2:
				log_info(logger, "Segundo analisis seteando bit uso en 0 y buscando MOD");
				if (pagina->bitPresencia)
				{
					if (pagina->bitUso == 0 && pagina->bitModificacion == 1)
					{
						pagina->bitModificacion = 0;
						void *aEnviar = malloc(tamanioPagina);
						memcpy(aEnviar,
							   memoriaCompleta + (pagina->marco * tamanioPagina),
							   tamanioPagina);
						log_info(logger, "Se enviara a Swamp la victima");
						enviarVictimaPageFault(proceso->id_proceso, encontrada->pagina, aEnviar);
						log_info(logger, "La posicion en la lista Clock es %d", posicionClock);
						posicionClock++;
						posicionClock = controlPosicionClock(posicionClock, tamanio);
						return encontrada;
					}
					else
					{
						pagina->bitUso = 0;
					}
				}
				break;
			default:
				break;
			}
		}
		posicionClock = 0;
	}

	return encontrada;
}
/*
Pagina* analizarPaginaClock(t_list* local,uint32_t pagina,t_list*paginas){
      Pagina*paginaEncontrada=list_get(paginas,pagina);
	  bool buscarPaginaMarco(void*elem){
		  Pagina* pagina=elem;
		  return pagina->marco==*marco;
	  }	
	  Pagina*aAnalizar=list_find(paginas,buscarPaginaMarco);
	  return aAnalizar;
}

referenciaGlobalClockLRU *analizarReferenciaGlobal(uint32_t marco)
{
	bool buscarReferenciaMarco(void *elem)
	{
		referenciaGlobalClockLRU *ref = elem;
		return ref->marco == marco;
	}

	referenciaGlobalClockLRU *encontrado = list_find(globalLRUCLOCK, buscarReferenciaMarco);
	referenciaGlobalPaginaClock = encontrado->pagina;
	referenciaGlobalProcesoClock = encontrado->id_carpincho;
	log_info(logger, "analizando CLOCK pagina: %d ID: %d", referenciaGlobalPaginaClock, referenciaGlobalProcesoClock);
	return encontrado;
}

uint32_t posicionListaGlobalCLOCKLRU()
{
	if (referenciaGlobalProcesoClock == 0 && referenciaGlobalPaginaClock == 0)
	{
		return 0;
	}
	else
	{

		for (int i = 0; i < list_size(globalLRUCLOCK); i++)
		{
			referenciaGlobalClockLRU *analizar = list_get(globalLRUCLOCK, i);
			if (analizar->pagina == referenciaGlobalPaginaClock &&
				analizar->id_carpincho == referenciaGlobalProcesoClock)
			{
				return i;
				break;
			}
		}
	}
}
*/
//=======================================Sustituciones No confundir====================================
void realizarSustitucionPagina(AdmProceso *proceso, uint32_t pagina, uint32_t cantidadPaginas)
{

	log_info(logger, "SE realizara %d sustituciones", cantidadPaginas);
	for (int i = 0; i < cantidadPaginas; i++)

	{
		pagina++;
		if (string_equals_ignore_case("FIJA", tipoAsignacion))
		{
			realizarSustitucionPaginaLocal(proceso, pagina);
		}
		else
		{
			realizarSustitucionPaginaGlobal(proceso, pagina);
		}
	}
}
void realizarSustitucionPaginaGlobal(AdmProceso *proceso, uint32_t pagina)
{
	if (string_equals_ignore_case("LRU", tipoSwap))
	{

		mostrarListaCLOCKLRU(globalLRUCLOCK);
		log_info(logger, "ALGORITMO :Aplicando algoritmo LRU");
		referenciaGlobalClockLRU *refVictima = list_remove(globalLRUCLOCK, 0);
		Marco *marcoAsignado = list_get(listaDeMarcos, refVictima->marco);
		marcoAsignado->esLibre = true;

		log_info(logger, "Se buscara TLB");

		TLB *buscada = actualizarListaTLBporAlgoritmo(refVictima->marco);
		if (buscada)
		{
			buscada->pagina = pagina;
			buscada->id = proceso->id_proceso;
		}
		AdmProceso *procesoVictima = buscarCarpincho(refVictima->id_carpincho);
		Pagina *paginaVictima = list_get(procesoVictima->tablaDePaginas, refVictima->pagina);
		log_info(logger, "Datos PAgina Marco %d ID %d", paginaVictima->marco, refVictima->id_carpincho);
		void *aEnviar = memoriaCompleta + (paginaVictima->marco * tamanioPagina);
		if (paginaVictima->bitModificacion)
		{
			log_info(logger, "SE envio la victima para escritura");
			enviarVictimaPageFault(refVictima->id_carpincho, refVictima->pagina, aEnviar);
		}
		else
		{
			log_info(logger, "No es necesario enviar no esta modificado");
		}
		paginaVictima->marco = -1;
		paginaVictima->bitModificacion = false;
		paginaVictima->bitUso = false;
		paginaVictima->bitPresencia = false;
		free(refVictima);
	}

	if (string_equals_ignore_case("CLOCK-M", tipoSwap))
	{
		mostrarListaCLOCKLRU(globalLRUCLOCK);
		log_info(logger, "ALGORITMO :Aplicando algoritmo CLOCK-M");
		referenciaGlobalClockLRU *refVictima = encontrarUnaVictimaGlobalClockM();
		log_info(logger, "Se desasigna el marco a la pagina");
		Marco *marcoAsignado = list_get(listaDeMarcos, refVictima->marco);
		marcoAsignado->esLibre = true;

		log_info(logger, "Se buscara TLB");

		TLB *buscada = actualizarListaTLBporAlgoritmo(refVictima->marco);
		if (buscada)
		{
			buscada->pagina = pagina;
			buscada->id = proceso->id_proceso;
		}
		AdmProceso *procesoVictima = buscarCarpincho(refVictima->id_carpincho);

		Pagina *paginaVictima = list_get(procesoVictima->tablaDePaginas, refVictima->pagina);
		log_info(logger, "Datos PAgina Marco %d ID %d", paginaVictima->marco, refVictima->id_carpincho);

		void *aEnviar = memoriaCompleta + (paginaVictima->marco * tamanioPagina);
		if (paginaVictima->bitModificacion)
		{
			log_info(logger, "SE envio la victima para escritura");
			enviarVictimaPageFault(refVictima->id_carpincho, refVictima->pagina, aEnviar);
		}
		else
		{
			log_info(logger, "No es necesario enviar no esta modificado");
		}

		paginaVictima->marco = -1;
		paginaVictima->bitModificacion = false;
		paginaVictima->bitUso = false;
		paginaVictima->bitPresencia = false;
		mostrarPaginasTotales(proceso->tablaDePaginas, proceso->id_proceso);
	}
}
void realizarSustitucionPaginaLocal(AdmProceso *proceso, uint32_t pagina)
{
	if (string_equals_ignore_case("LRU", tipoSwap))
	{

		log_info(logger, "ALGORITMO :Aplicando algoritmo LRU");
		uint32_t *victima = (uint32_t)encontrarUnaVictimaLocalLRU(proceso);
		log_info(logger, "Modificar y preparar la pagina solicitada");

		TLB *buscada = actualizarListaTLBporAlgoritmo(*victima);
		Pagina *paginaVictima = list_get(proceso->tablaDePaginas, buscada->pagina);
		void *aEnviar = memoriaCompleta + (paginaVictima->marco * tamanioPagina);

		Marco *marcoAsignado = list_get(listaDeMarcos, *victima);
		marcoAsignado->esLibre = true;
		log_info(logger, "Desasignando marco Marco %d EsLibre %d ", *victima, marcoAsignado->esLibre);

		if (paginaVictima->bitModificacion)
		{
			log_info(logger, "SE envio la victima para escritura");
			enviarVictimaPageFault(proceso->id_proceso, buscada->pagina, aEnviar);
		}
		else
		{
			log_info(logger, "No es necesario enviar no esta modificado");
		}
		paginaVictima->marco = -1;
		paginaVictima->bitModificacion = false;
		paginaVictima->bitUso = false;
		paginaVictima->bitPresencia = false;

		buscada->pagina = pagina;
	}

	if (string_equals_ignore_case("CLOCK-M", tipoSwap))
	{
		log_info(logger, "ALGORITMO :Aplicando algoritmo LRU");
		uint32_t victima = (uint32_t)encontrarUnaVictimaLocalClockM(proceso);
		log_info(logger, "Modificar y preparar la pagina solicitada");

		TLB *buscada = actualizarListaTLBporAlgoritmo(victima);
		Pagina *paginaVictima = list_get(proceso->tablaDePaginas, buscada->pagina);
		void *aEnviar = memoriaCompleta + (paginaVictima->marco * tamanioPagina);

		Marco *marcoAsignado = list_get(listaDeMarcos, victima);
		marcoAsignado->esLibre = true;
		log_info(logger, "Desasignando marco Marco %d EsLibre %d ", victima, marcoAsignado->esLibre);

		if (paginaVictima->bitModificacion)
		{
			log_info(logger, "SE envio la victima para escritura");
			enviarVictimaPageFault(proceso->id_proceso, buscada->pagina, aEnviar);
		}
		else
		{
			log_info(logger, "No es necesario enviar no esta modificado");
		}
		paginaVictima->marco = -1;
		paginaVictima->bitModificacion = false;
		paginaVictima->bitUso = false;
		paginaVictima->bitPresencia = false;

		buscada->pagina = pagina;
	}
}
//====================================================Swapeos y pagefaults=====================================
void *realizarSwapLocal(AdmProceso *proceso, uint16_t *paginaVictima,
						uint32_t *pidVictima, uint16_t pagina)
{

	if (string_equals_ignore_case("LRU", tipoSwap))
	{
		log_info(logger, "ALGORITMO :Aplicando algoritmo LRU");
		uint32_t *victima = (uint32_t)encontrarUnaVictimaLocalLRU(proceso);
		log_info(logger, "Modificar y preparar la pagina solicitada");

		TLB *buscada = actualizarListaTLBporAlgoritmo(*victima);
		log_info(logger, "Datos TLB Pagina %d  Marco %d id %d", buscada->pagina, buscada->marco, buscada->id);
		Pagina *paginaSolicitada = list_get(proceso->tablaDePaginas, pagina);
		paginaSolicitada->bitPresencia = true;
		paginaSolicitada->marco = buscada->marco;
		paginaSolicitada->bitUso = true;
		log_info(logger, "Obtener puntero del marco %d", buscada->marco);
		void *punteroMarco = memoriaCompleta + (paginaSolicitada->marco * tamanioPagina);
		log_info(logger, "Agregar a la lista LRU pagina solicitada %d", pagina);

		list_add(proceso->localLRUCLOCK, victima);

		mostrarListaCLOCKLRU(proceso->localLRUCLOCK);
		(*paginaVictima) = buscada->pagina;
		(*pidVictima) = proceso->id_proceso;
		buscada->pagina = pagina;
		return punteroMarco;
	}
	if (string_equals_ignore_case("CLOCK-M", tipoSwap))
	{
		log_info(logger, "ALGORITMO :Aplicando algoritmo cLOCK");
		uint32_t victima = encontrarUnaVictimaLocalClockM(proceso);
		log_info(logger, "El marco victima es %d", victima);

		Pagina *paginaSolicitada = list_get(proceso->tablaDePaginas, pagina);
		paginaSolicitada->bitPresencia = true;
		paginaSolicitada->marco = victima;
		paginaSolicitada->bitUso = true;
		TLB *buscada = actualizarListaTLBporAlgoritmo(victima);
		void *punteroMarco = memoriaCompleta + (victima * tamanioPagina);
		log_info(logger, "Datos TLB: marco %d pagina %d id %d", buscada->marco,
				 buscada->pagina, buscada->id);
		(*paginaVictima) = buscada->pagina;
		(*pidVictima) = proceso->id_proceso;

		buscada->pagina = pagina;

		return punteroMarco;
	}
	return NULL;
}
void *realizarSwapGlobal(t_list *paginas, uint16_t *paginaVictima,
						 uint32_t *pidVictima, uint16_t pagina, uint32_t id)
{
	int victima = 0;

	if (string_equals_ignore_case("LRU", tipoSwap))
	{
		mostrarListaCLOCKLRU(globalLRUCLOCK);
		log_info(logger, "ALGORITMO :Aplicando algoritmo LRU");
		referenciaGlobalClockLRU *refVictima = list_remove(globalLRUCLOCK, 0);

		log_info(logger, "Se buscara TLB");

		TLB *buscada = actualizarListaTLBporAlgoritmo(refVictima->marco);
		if (buscada)
		{
			buscada->pagina = pagina;
			buscada->id = id;
		}

		(*paginaVictima) = refVictima->pagina;
		(*pidVictima) = refVictima->id_carpincho;
		log_info(logger, "Se modificara pagina");

		Pagina *paginaSolicitada = list_get(paginas, pagina);

		paginaSolicitada->bitPresencia = true;
		paginaSolicitada->marco = refVictima->marco;
		paginaSolicitada->bitUso = true;
		log_info(logger, "PAgina solicitada Marco: %d",
				 paginaSolicitada->marco);
		log_info(logger, "Agregar a la lista LRU marco solicitada %d",
				 refVictima->marco);
		refVictima->pagina = pagina;
		refVictima->id_carpincho = id;

		list_add(globalLRUCLOCK, refVictima);

		mostrarListaCLOCKLRU(globalLRUCLOCK);

		mostrarListaTLBs();
		mostrarPaginasTotales(paginas, id);
		void *aEnviar = memoriaCompleta + (refVictima->marco * tamanioPagina);

		return aEnviar;
	}
	if (string_equals_ignore_case("CLOCK-M", tipoSwap))
	{
		log_info(logger, "ALGORITMO :Aplicando algoritmo cLOCK");
		referenciaGlobalClockLRU *refVictima = encontrarUnaVictimaGlobalClockM();
		log_info(logger, "El marco victima es %d", refVictima->marco);

		TLB *buscada = actualizarListaTLBporAlgoritmo(refVictima->marco);
		if (buscada)
		{
			buscada->pagina = pagina;
			buscada->id = id;
		}
		Pagina *paginaSolicitada = list_get(paginas,
											pagina);
		paginaSolicitada->bitPresencia = true;
		paginaSolicitada->marco = refVictima->marco;
		paginaSolicitada->bitUso = true;
		void *punteroMarco = memoriaCompleta + (refVictima->marco * tamanioPagina);
		(*paginaVictima) = refVictima->pagina;
		(*pidVictima) = refVictima->id_carpincho;
		log_info(logger, "Se le sustituiran estos Datos  pagina %d ID %d a La victima Pagina %d ID %d", pagina, id, refVictima->pagina, refVictima->id_carpincho);
		refVictima->pagina = pagina;
		refVictima->id_carpincho = id;

		mostrarListaCLOCKLRU(globalLRUCLOCK);
		mostrarListaTLBs();
		mostrarPaginasTotales(paginas, id);

		return punteroMarco;
	}
	return NULL;
}
void mostrarListaLRUCLOCKglobal()
{
}
void actualizarParametrosPAraAlgoritmos(Pagina *pagina, AdmProceso *proceso)
{
	bool sacarMarcoDeLalistaGlobal(void *elem)
	{
		referenciaGlobalClockLRU *referencia = elem;
		return referencia->marco == pagina->marco;
	}
	if (string_equals_ignore_case("LRU", tipoSwap))
	{
		actualizadorVictimasLRU(pagina, proceso->id_proceso, list_size(proceso->tablaDePaginas) - 1,
								proceso->localLRUCLOCK);

		pagina->bitUso = true;
		pagina->bitModificacion = true;
	}
	else
	{

		referenciaGlobalClockLRU *encontrada = list_find(globalLRUCLOCK, sacarMarcoDeLalistaGlobal);
		if (encontrada == NULL)
		{
			referenciaGlobalClockLRU *referencia = malloc(sizeof(referenciaGlobalClockLRU));
			referencia->marco = pagina->marco;
			referencia->pagina = list_size(proceso->tablaDePaginas) - 1;
			referencia->id_carpincho = proceso->id_proceso;
			//list_remove_by_condition(globalLRU, sacarMarcoDeLalista);
			list_add(globalLRUCLOCK, referencia);
			pagina->bitUso = true;
			pagina->bitModificacion = true;
		}
		else
		{
			encontrada->pagina = list_size(proceso->tablaDePaginas) - 1;
			encontrada->id_carpincho = proceso->id_proceso;
			pagina->bitUso = true;
			pagina->bitModificacion = true;
		}
	}
}
void actualizarSelectorVictima(Pagina *actualizar, AdmProceso *proceso,
							   uint32_t dirLog, bool escritura)
{
	bool encontrarParaRemoverLocal(void *numPagina)
	{
		int *num = numPagina;
		return obtenerDeterminadaPagina(dirLog) == *num;
	}
	bool encontrarParaRemoverGlobal(void *numMarco)
	{
		int *num = numMarco;
		return actualizar->marco == *num;
	}
	if (string_equals_ignore_case(tipoAsignacion, "FIJA"))
	{
		if (string_equals_ignore_case(tipoSwap, "LRU"))
		{
			int *primero = list_remove_by_condition(proceso->localLRUCLOCK,
													encontrarParaRemoverLocal);
			list_add(proceso->localLRUCLOCK, primero);
			if (escritura)
			{
				actualizar->bitModificacion = true;
			}
			else
			{
				actualizar->bitUso = true;
			}
		}
		else
		{
			if (escritura)
			{
				actualizar->bitModificacion = true;
			}
			else
			{
				actualizar->bitUso = true;
			}
		}
	}
	else
	{
		if (string_equals_ignore_case(tipoSwap, "LRU"))
		{
			int *primero = list_remove_by_condition(proceso->localLRUCLOCK,
													encontrarParaRemoverGlobal);
			;
			list_add(globalLRUCLOCK, primero);
		}
		else
		{
			if (escritura)
			{
				actualizar->bitModificacion = true;
			}
			else
			{
				actualizar->bitUso = true;
			}
		}
	}
}

void iniciarAdministradorDeVictimasLRU(Pagina *pag, int pos, uint32_t id,
									   t_list *posicionesLRU)
{
	bool sacarMarcoDeLalista(void *elem)
	{
		uint32_t *marco = elem;
		return pag->marco == *marco;
	}

	if (string_equals_ignore_case(tipoAsignacion, "FIJA"))
	{
		//list_remove_by_condition(posicionesLRU, sacarMarcoDeLalista);
		//uint32_t *marcoP=&pag->marco;

		uint32_t *marco = list_remove(posicionesLRU, 0);
		log_info(logger, "Se actualizo la lista LRU/CLOCK con Marco %d", *marco);
		list_add(posicionesLRU, marco);
	}
	else
	{

		referenciaGlobalClockLRU *referencia = malloc(sizeof(referenciaGlobalClockLRU));
		referencia->marco = pag->marco;
		referencia->pagina = pos;
		referencia->id_carpincho = id;
		//list_remove_by_condition(globalLRU, sacarMarcoDeLalista);
		list_add(globalLRUCLOCK, referencia);
	}
}
void actualizadorVictimasLRU(Pagina *pagina, uint32_t id, uint32_t pos, t_list *localLRU)
{
	bool sacarMarcoDeLalista(void *elem)
	{
		uint32_t *marco = elem;
		return pagina->marco == *marco;
	}
	bool sacarMarcoDeLalistaGlobal(void *elem)
	{
		referenciaGlobalClockLRU *referencia = elem;
		return referencia->marco == pagina->marco;
	}
	if (string_equals_ignore_case(tipoAsignacion, "FIJA"))
	{
		uint32_t *marco = list_remove_by_condition(localLRU, sacarMarcoDeLalista);

		list_add(localLRU, marco);
	}
	else
	{
		referenciaGlobalClockLRU *encontrada = list_remove_by_condition(globalLRUCLOCK, sacarMarcoDeLalistaGlobal);
		if (encontrada == NULL)
		{
			referenciaGlobalClockLRU *referencia = malloc(sizeof(referenciaGlobalClockLRU));
			referencia->marco = pagina->marco;
			referencia->pagina = pos;
			referencia->id_carpincho = id;
			//list_remove_by_condition(globalLRU, sacarMarcoDeLalista);
			list_add(globalLRUCLOCK, referencia);
		}
		else
		{
			list_add(globalLRUCLOCK, encontrada);
		}
		//list_remove_by_condition(globalLRU, sacarMarcoDeLalista);
	}
}
void recuperacionPaginaFaltante(uint32_t pid, uint16_t pagina)
{
	uint16_t paginaVictima;
	uint32_t pidVictima;
	AdmProceso *carp = buscarCarpincho(pid);

	void *ptr_Marco;

	bool cantidadPaginasconMarco(void *elem)
	{
		Pagina *presente = elem;
		return presente->marco != -1;
	}
	log_info(logger,
			 "COMENZANDO PROCEDIMIENTO DE PAGEFAULT: verificando tipo de asignacion y algoritmo");

	if (string_equals_ignore_case("FIJA", tipoAsignacion))
	{
		log_info(logger, "Asignacion FIJA");
		ptr_Marco = realizarSwapLocal(carp, &paginaVictima, &pidVictima,
									  pagina);
	}
	else
	{
		log_info(logger, "Asignacion GLOBAL");
		ptr_Marco = realizarSwapGlobal(carp->tablaDePaginas, &paginaVictima,
									   &pidVictima, pagina, pid);
	}

	log_info(logger,
			 "PROCEDIMIENTO DE PAGEFAULT: se obtuvo el puntero al marco victima");

	void *aSwapear = malloc(tamanioPagina);

	memcpy(aSwapear, ptr_Marco, tamanioPagina);

	log_info(logger,
			 "PROCEDIMIENTO DE PAGEFAULT: copiar y enviar eel marco victima si es necesario");
	log_info(logger,
			 "Pagina Victima %d, pid victima %d, pagina solicitada %d, pid solicitada %d",
			 paginaVictima, pidVictima, pagina, pid);

	void *aPisar = analizarTipoPageFault(paginaVictima, pidVictima, pagina, pid,
										 aSwapear);
	HeapMetadata *comprobar = aPisar;
	log_info(logger, "HeapMetadata comprobar Previo  %d",
			 comprobar->prevAlloc);
	log_info(logger, "HeapMetadata comprobar Siguiente %d",
			 comprobar->nextAlloc);

	log_info(logger,
			 "PROCEDIMIENTO DE PAGEFAULT: se obtuvo la página solicitada");

	memcpy(ptr_Marco, aPisar, tamanioPagina);

	log_info(logger,
			 "PROCEDIMIENTO DE PAGEFAULT: se remplazo el marco victima con el marco solicitado");

	/*
	if (list_count_satisfying(carp->tablaDePaginas, cantidadPaginasconMarco) ==
			marcosPorCarpincho &&
		string_equals_ignore_case("FIJA", tipoAsignacion))
	{
		uint32_t marcoFijo = obtenerMarcoLibreFijo(carp->localLRUCLOCK);
		void *ptr_marco_solicitado = memoriaCompleta + (marcoFijo * tamanioPagina);
		Pagina *solicitada = list_get(carp->tablaDePaginas, pagina);
		solicitada->marco = marcoFijo;
		solicitada->bitPresencia = true;
		solicitada->bitUso = true;
		void *aPisar = solicitarPaginaPageFault(pagina, pid);

		log_info(logger,
				 "PROCEDIMIENTO DE PAGEFAULT: se obtuvo la página solicitada");

		memcpy(ptr_marco_solicitado, aPisar, tamanioPagina);
	}
	else
	{

		
	}
	*/
}
void *analizarTipoPageFault(uint16_t pagVictima, uint32_t pidVictima,
							uint16_t paginaSolicitada, uint32_t pidSolicitada, void *aSwapear)
{

	AdmProceso *proceso = buscarCarpincho(pidVictima);
	Pagina *victima = list_get(proceso->tablaDePaginas, pagVictima);
	if (victima->bitModificacion)
	{
		log_info(logger, "SELECTOR DE PAGEFAULT: pageFault con envio de victima");
		victima->marco = -1;
		victima->bitModificacion = false;
		victima->bitUso = false;
		victima->bitPresencia = false;
		enviarVictimaPageFault(pidVictima, pagVictima, aSwapear);
		return solicitarPaginaPageFault(paginaSolicitada, pidSolicitada);
	}
	else
	{
		log_info(logger, "SELECTOR DE PAGEFAULT: pageFault solicitando pagina");
		victima->marco = -1;
		victima->bitUso = false;
		victima->bitPresencia = false;
		return solicitarPaginaPageFault(paginaSolicitada, pidSolicitada);
	}
}

void retornarTodasLasPaginasFaltantes(uint32_t alloc, t_list *paginas,
									  uint32_t pid)
{
	AdmProceso* proceso=buscarCarpincho(pid);
	HeapMetadata *inicio;
	if (obtenerResto(alloc < 9))
	{
		inicio = entreDosPaginas(alloc, proceso);
		recuperacionPaginaFaltante(pid, obtenerDeterminadaPagina(alloc) - 1);
	}
	else
	{
		inicio = ptr_marco(obtenerDeterminadaPagina(alloc), paginas) + (obtenerResto(alloc) - sizeof(HeapMetadata));
	}
	uint32_t paginaTecho = obtenerDeterminadaPagina(inicio->nextAlloc);
	for (int i = obtenerDeterminadaPagina(alloc); i <= paginaTecho; i++)
	{
		Pagina *enMemoria = list_get(paginas, i);
		if (enMemoria->bitPresencia)
		{
			log_info(logger, "Pagina presente en memoria");
		}
		else
		{
			recuperacionPaginaFaltante(pid, i);
		}
	}
}
//---------------------------Sistemas de metricas--------------------------------
void *sistemaDeMetricas()
{

	signal(SIGINT, &tlbHitandMiss);
	signal(SIGUSR1, &dumpMemoria);
	//signal(SIGUSR2,&notificarEmergencia);
	while (1)
	{
		printf("No hay emergencia");
		sleep(60);
	}
}

void dumpMemoria()
{

	if (fopen("Dump_<Timestamp>.tlb", "r"))
	{
		log_info(logger, "BitMap existe");
	}
	else
	{
		FILE *fl = fopen("Dump_<Timestamp>.tlb", "w+");
		log_info(logger, "Se creo bitMap con exito");
	}
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	log_info(logger, "Dump: %d/%d/%d %d:%d:%d", tm.tm_year + 1900,
			 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	int entradas = 0;
	for (int i = 0; i < list_size(listaDeProcesos); i++)
	{
		AdmProceso *proceso = list_get(listaDeProcesos, i);
		for (int j = 0; j < list_size(proceso->tablaDePaginas); j++)
		{
			Pagina *dumping = list_get(proceso->tablaDePaginas, j);
			log_info(logger,
					 "Entrada: %d  Estado: %s  Carpincho: %d Pagina: %d  Marco: %d",
					 entradas, estadoPagina(dumping->bitPresencia),
					 proceso->id_proceso, j, dumping->marco);
			entradas++;
		}
	}
}
char *estadoPagina(bool estado)
{
	if (estado)
	{
		return "ocupado";
	}
	else
	{
		return "libre";
	}
}
int cantidadEntradas()
{
	int cantidad = 0;
	for (int i = 0; i < list_size(listaDeProcesos); i++)
	{
		AdmProceso *proceso = list_get(listaDeProcesos, i);
		cantidad += list_size(proceso->tablaDePaginas);
	}
	return cantidad;
}
void tlbHitandMiss()
{

	int totalesHit = 0;
	int totalesMiss = 0;
	for (int i = 0; i < list_size(listaDeProcesos); i++)
	{
		AdmProceso *proceso = list_get(listaDeProcesos, i);
		int HitsCarpincho = metricasHitTLB(proceso);
		int MissCarpincho = metricasHitTLB(proceso);
		log_info(logger, "Los Hits del carpincho %d son %d",
				 proceso->id_proceso, HitsCarpincho);
		log_info(logger, "Los Miss del carpincho %d son %d",
				 proceso->id_proceso, HitsCarpincho);
		totalesHit += HitsCarpincho;
		totalesMiss += MissCarpincho;
	}
	log_info(logger, "Los Hits totales son %d", totalesHit);
	log_info(logger, "Los Miss totalse son %d", totalesMiss);
}
int metricasHitTLB(AdmProceso *proceso)
{

	int hits = 0;
	for (int i = 0; i < list_size(proceso->tablaDePaginas); i++)
	{
		Pagina *analizar = list_get(proceso->tablaDePaginas, i);
		if (analizar->bitPresencia)
		{
			hits++;
		}
	}
	return hits;
}
int metricasMissTLB(AdmProceso *proceso)
{

	int miss = 0;
	for (int i = 0; i < list_size(proceso->tablaDePaginas); i++)
	{
		Pagina *analizar = list_get(proceso->tablaDePaginas, i);
		if (analizar->bitPresencia == false)
		{
			miss++;
		}
	}
	return miss;
}

//----------------------Funciones complementarias----------------------

void comprobarAplicacionDeSustitucion(AdmProceso *proceso, uint32_t cantidadDePagina)
{
	bool cantidadPresentesMemoria(void *elem)
	{
		Pagina *pag = elem;
		return pag->bitPresencia;
	}

	if (list_count_satisfying(proceso->tablaDePaginas, cantidadPresentesMemoria) == marcosPorCarpincho && string_equals_ignore_case("FIJA", tipoAsignacion))
	{
		log_info(logger,
				 "LGORITMO DE REMPLAZO MEDIANTE TLB: se procedera a aplicar el algoritmo correspondiente");
		aplicarAlgoritmoRemplazoTLB(proceso->id_proceso,
									cantidadDePagina, proceso->tablaDePaginas,
									proceso->localLRUCLOCK);
	}
	else if (hayMarcosLibres() == false)
	{
		/*En caso global cuando la memoria este llena se verificara enviando un mensaje a Swap
			 si aun queda espacio en el archivo asignado al carpincho, se reservara los marcos y se
			 realizara la sustitucion mediante la TLB
			 */
		log_info(logger,
				 "LGORITMO DE REMPLAZO MEDIANTE TLB GLOBAL : se procedera a aplicar el algoritmo correspondiente");
		aplicarAlgoritmoRemplazoTLBGlobal(cantidadDePagina);
	}
}

int paginasPresentes(void *elem)
{
	Pagina *pag = elem;
	return pag->bitPresencia;
}
void *buscarMarcoDirLog(uint32_t dirLog, t_list *paginas)
{
	Pagina *encontrada = list_get(paginas, obtenerDeterminadaPagina(dirLog));
	Marco *utilizado = list_get(listaDeMarcos, encontrada->marco);
	return utilizado->marco;
}

uint16_t obtenerDeterminadaPagina(uint32_t dirLog)
{
	if (dirLog < tamanioPagina)
	{
		return 0;
	}
	float cantidad = dirLog / tamanioPagina;
	int resta = 1 + dirLog / tamanioPagina;
	int cantPag = (int)(cantidad + (resta - cantidad));
	return cantPag - 1;
}

uint32_t obtenerResto(uint32_t dirLog)
{
	if (dirLog < tamanioPagina)
	{
		return dirLog;
	}
	int resto = dirLog % tamanioPagina;
	return resto;
}
uint32_t obtenerDesplazamiento(uint32_t dirLog)
{
	int desplazamiento = (obtenerDeterminadaPagina(dirLog) * tamanioPagina) + obtenerResto(dirLog);
	return desplazamiento;
}

int obtenerMarcoLibre()
{
	int contador = list_size(listaDeMarcos);
	for (int i = 0; i < contador; i++)
	{
		Marco *aux = list_get(listaDeMarcos, i);
		if (aux->esLibre)
		{
			log_info(logger, "El marco libre encontrado es %d", i);
			aux->esLibre = false;
			return i;
		}
	}
	return -1;
}
int obtenerMarcoLibreFIJA()
{
	int contador = list_size(listaDeMarcos);
	for (int i = 0; i < contador; i++)
	{
		Marco *aux = list_get(listaDeMarcos, i);
		if (aux->esLibre)
		{
			log_info(logger, "El marco libre encontrado es %d", i);

			return i;
		}
	}
	return -1;
}
HeapMetadata *crearHeapMetadata()
{
	HeapMetadata *nuevo = malloc(sizeof(HeapMetadata));
	nuevo->prevAlloc = -1;
	nuevo->nextAlloc = -1;
	nuevo->esLibre = 0;

	return nuevo;
}

int posibleAsignacion()
{
	int cantidad = list_size(listaDeProcesos);
	int cantidadPaginas = 0;
	for (int i = 0; i < cantidad; i++)
	{
		AdmProceso *tlb = list_get(listaDeProcesos, i);
		cantidadPaginas += list_size(tlb->tablaDePaginas);
	}
	log_info(logger, "La cantidad de paginas es %d", cantidadPaginas);
	if (cantidadPaginas < (tamanioMemoria / tamanioPagina))
	{
		return 1;
	}
	return 0;
}

Pagina *devolverPaginaVerificarSwamp(Pagina *pagina, AdmProceso *proceso, uint32_t alloc, uint32_t nextalloc)
{
	Pagina *aux;
	if (obtenerDeterminadaPagina(alloc) == obtenerDeterminadaPagina(nextalloc))
	{
		log_info(logger, "El siguiente heap esta en esta pagina");
		return pagina;
	}
	else
	{
		log_info(logger, "El siguiente heap esta en otra pagina");
		Pagina *aux = list_get(proceso->tablaDePaginas, obtenerDeterminadaPagina(nextalloc));
		if (aux->bitPresencia == false)
		{
			recuperacionPaginaFaltante(proceso->id_proceso, obtenerDeterminadaPagina(nextalloc));
		}
		return aux;
	}
	return aux;
}
int cantidadPaginas(int size)
{
	if (size % tamanioPagina == 0)
	{
		return size / tamanioPagina;
	}
	double cantidad = size / tamanioPagina;
	int resta = 1 + size / tamanioPagina;
	int cantPag = (int)(cantidad + (resta - cantidad));
	return cantPag;
}

Pagina *crearPagina()
{
	Pagina *nuevo = malloc(sizeof(Pagina));
	nuevo->bitPresencia = true;
	nuevo->bitModificacion = false;
	nuevo->bitUso = false;
	nuevo->size = 0;
	return nuevo;
}

bool direccionLogicaValida(AdmProceso *proceso, uint32_t dirLog)
{
	if (list_get(proceso->tablaDePaginas,
				 obtenerDeterminadaPagina(dirLog)) == NULL)
	{
		return true;
	}
	return false;
}

void mostrarPaginasTotales(t_list *paginas, uint32_t id)
{

	log_info(logger,
			 "=====================================Paginas %d======================================", id);
	for (int i = 0; i < list_size(paginas); i++)
	{
		Pagina *pag = list_get(paginas, i);
		log_info(logger,
				 "Pagina %d BitPrecen %d BitMod %d BitUso %d, Marco %d ocupado %d",
				 i, pag->bitPresencia, pag->bitModificacion, pag->bitUso,
				 pag->marco, pag->size);
	}
	log_info(logger,
			 "=================================================================");
}

bool paginaPresenteEnMemoria(t_list *paginas, int pagina)
{
	Pagina *comprobar = list_get(paginas, pagina);
	return comprobar->bitPresencia;
}

AdmProceso *buscarCarpincho(uint32_t pid)
{
	int cantidad = list_size(listaDeProcesos);
	for (int i = 0; i < cantidad; i++)
	{
		AdmProceso *aux = list_get(listaDeProcesos, i);
		if (aux->id_proceso == pid)
		{
			return aux;
		}
	}
	return NULL;
}

/*
void modificarEntreDosPaginas(uint32_t dirLog, AdmProceso*proceso,uint32_t nuevoNext){
	uint32_t numeroPagina = obtenerDeterminadaPagina(dirLog);
	log_info(logger, "NumeroPagina del comienzo del HEAP %d", numeroPagina);
	Pagina *primera = list_get(proceso->tablaDePaginas, numeroPagina);
	Pagina *segunda = list_get(proceso->tablaDePaginas, (numeroPagina + 1));
	void *dosPaginas = malloc(tamanioPagina * 2);

	if (primera->bitPresencia == false)
	{
		log_info(logger, "Pagina no esta Presente en memoria");
		recuperacionPaginaFaltante(proceso->id_proceso, numeroPagina);
	}
	else
	{
		log_info(logger, "Pagina Presente en memoria");
	}
	void *ptrMarcoPrimero = memoriaCompleta + (primera->marco * tamanioPagina);
	if (segunda->bitPresencia == false)
	{
		log_info(logger, "Pagina no esta Presente en memoria");
		recuperacionPaginaFaltante(proceso->id_proceso, numeroPagina + 1);
	}
	else
	{
		log_info(logger, "Pagina Presente en memoria");
	}
	void *ptrMarcoSegundo = memoriaCompleta + (segunda->marco * tamanioPagina);
	memcpy(dosPaginas, ptrMarcoPrimero, tamanioPagina);
	memcpy(dosPaginas + tamanioPagina, ptrMarcoSegundo, tamanioPagina);

	uint32_t resto = obtenerResto(dirLog);
	log_info(logger, "Resto de dir Log %d", resto);

	HeapMetadata* heapModificar=dosPaginas+resto;
	heapModificar->nextalloc+=nuevoNext;
	heapModificar->esLibre=false;
	
}
*/

HeapMetadata *entreDosPaginas(uint32_t dirLog, AdmProceso*proceso)
{

	uint32_t numeroPagina = obtenerDeterminadaPagina(dirLog);
	log_info(logger, "NumeroPagina del comienzo del HEAP %d", numeroPagina);
	Pagina *primera = list_get(proceso->tablaDePaginas, numeroPagina);
	Pagina *segunda = list_get(proceso->tablaDePaginas, (numeroPagina + 1));
	void *dosPaginas = malloc(tamanioPagina * 2);

	if (primera->bitPresencia == false)
	{
		log_info(logger, "Pagina no esta Presente en memoria");
		recuperacionPaginaFaltante(proceso->id_proceso, numeroPagina);
	}
	else
	{
		log_info(logger, "Pagina Presente en memoria");
	}
	void *ptrMarcoPrimero = memoriaCompleta + (primera->marco * tamanioPagina);
	if (segunda->bitPresencia == false)
	{
		log_info(logger, "Pagina no esta Presente en memoria");
		recuperacionPaginaFaltante(proceso->id_proceso, numeroPagina + 1);
	}
	else
	{
		log_info(logger, "Pagina Presente en memoria");
	}
	void *ptrMarcoSegundo = memoriaCompleta + (segunda->marco * tamanioPagina);
	memcpy(dosPaginas, ptrMarcoPrimero, tamanioPagina);
	memcpy(dosPaginas + tamanioPagina, ptrMarcoSegundo, tamanioPagina);

	uint32_t resto = obtenerResto(dirLog);
	log_info(logger, "Resto de dir Log %d", resto);

	HeapMetadata *inicial = malloc(sizeof(HeapMetadata));
	memcpy(inicial, dosPaginas + resto, sizeof(HeapMetadata));
	log_info(logger, "Alloc previo entre dos Paginas %d", inicial->prevAlloc);
	log_info(logger, "Alloc siguiente entre dos Paginas %d",
			 inicial->nextAlloc);
	free(dosPaginas);

	return inicial;
}

//Son iguales, buscar una forma que evite usar estas dos funciones y solo una.

uint32_t allocarPrimero(int size, t_list *paginas, void *reservado)
{

	int desplazamiento = 0;
	for (int i = 0; i < list_size(paginas); i++)
	{
		HeapMetadata *aux = reservado;
		if (aux->esLibre == 1 && aux->nextAlloc - desplazamiento <= size)
		{
			return desplazamiento;
		}
		desplazamiento = aux->nextAlloc;
	}
	return desplazamiento;
}

uint32_t hayParaAllocar(int size, t_list *paginas, void *reservado)
{

	int desplazamiento = 0;
	int siguiente = 0;
	HeapMetadata *aux = reservado;
	desplazamiento += sizeof(HeapMetadata);
	siguiente = aux->nextAlloc;
	while (siguiente != -1)
	{
		if (aux->esLibre == true && aux->nextAlloc - (desplazamiento + sizeof(HeapMetadata)) >= size)
		{
			return desplazamiento;
		}
		desplazamiento = aux->nextAlloc;
		aux = reservado + (aux->nextAlloc - sizeof(HeapMetadata));
		siguiente = aux->nextAlloc;
	}
	return -1;
}

void allocarEnAllocLibre(int size, AdmProceso *proceso, uint32_t alloc)
{

	Pagina *inicial = list_get(proceso->tablaDePaginas, obtenerDeterminadaPagina(alloc));

	HeapMetadata *inicio = memoriaCompleta + (inicial->marco * tamanioPagina) + obtenerResto(alloc);
	//void *marco = memoriaCompleta + (inicial->marco * tamanioPagina);
	log_info(logger, "Alloc previo  %d", inicio->prevAlloc);
	log_info(logger, "Alloc siguiente %d", inicio->nextAlloc);
	uint32_t capacidad = inicio->nextAlloc - (alloc + sizeof(HeapMetadata));
	log_info(logger, "Capacidad libre del alloc %d", capacidad);
	if (size == capacidad)
	{
		log_info(logger, "la reserva cabe perfectamente en el alloc libre");
		inicio->esLibre = false;
		Pagina *paginaFinal = devolverPaginaVerificarSwamp(inicial, proceso,
														   alloc, inicio->nextAlloc);

		paginaFinal->bitModificacion = true;
		paginaFinal->bitUso = true;
		paginaFinal->size = size;
	}
	else
	{
		log_info(logger, "Alloc demasiado grande creando HeapMetadata auxiliar");

		uint32_t nuevaDirLog = alloc + (size + sizeof(HeapMetadata));
		HeapMetadata *finalHeap = crearHeapMetadata();
		finalHeap->prevAlloc = alloc;
		finalHeap->nextAlloc = inicio->nextAlloc;
		finalHeap->esLibre = false;

		Pagina *siguiente = devolverPaginaVerificarSwamp(inicial, proceso,
														 alloc, inicio->nextAlloc);
		HeapMetadata *heapSiguiente = memoriaCompleta + (siguiente->marco * tamanioPagina) +
									  obtenerResto(inicio->nextAlloc);

		heapSiguiente->prevAlloc = alloc + (size + sizeof(HeapMetadata));
		heapSiguiente->esLibre = false;
		Pagina *escribir = devolverPaginaVerificarSwamp(inicial, proceso,
														alloc, nuevaDirLog);
		escribir->bitModificacion = true;
		escribir->bitUso = true;
		escribir->size = size;

		void *comienzo = memoriaCompleta + (escribir->marco * tamanioPagina) +
						 +obtenerResto(nuevaDirLog);
		memcpy(comienzo, finalHeap, sizeof(HeapMetadata));

		inicio->nextAlloc = alloc + (size + sizeof(HeapMetadata));

		log_info(logger, "Nuevo heap prev %d", finalHeap->prevAlloc);
		log_info(logger, "Nuevo heap next %d", finalHeap->nextAlloc);

		log_info(logger, "inicial heap prev %d", inicio->prevAlloc);
		log_info(logger, "Inicial heap next %d", inicio->nextAlloc);

		log_info(logger, "siguiente heap prev %d", heapSiguiente->prevAlloc);
		log_info(logger, "siguiente heap next %d", heapSiguiente->nextAlloc);
	}
}

//===================================================Funcion para allocar en alloc libre===========================
void pisarDesdePaginaPagina(uint32_t paginaInicial, uint32_t paginaFinal,
							t_list *paginas, void *aPisar)
{
	uint32_t desplazamiento = 0;
	for (int i = paginaInicial; i <= paginaFinal; i++)
	{
		Pagina *aux = list_get(paginas, i);

		memcpy(memoriaCompleta + (aux->marco + tamanioPagina),
			   aPisar + (aux->marco + tamanioPagina), tamanioPagina);
		desplazamiento += tamanioPagina;
	}
}

uint32_t hayAllocLibre(int size, uint32_t pid, t_list *paginas)
{
	int desplazamiento = 0;
	int siguiente = 0;
	AdmProceso*proceso=buscarCarpincho(pid);

	Pagina *pagina = list_get(paginas, obtenerDeterminadaPagina(desplazamiento));
	if (pagina->bitPresencia == false)
	{
		recuperacionPaginaFaltante(pid,
								   (uint16_t)obtenerDeterminadaPagina(desplazamiento));
	}

	log_info(logger, "Pagina marco %d", pagina->marco);

	HeapMetadata *aux = memoriaCompleta + ((pagina->marco * tamanioPagina) + obtenerResto(siguiente));

	while (siguiente != -1)
	{

		log_info(logger, "HeapMetadata aux prev %d", aux->prevAlloc);
		log_info(logger, "HeapMetadata aux next %d", aux->nextAlloc);
		log_info(logger, "Estado %d", aux->esLibre);
		log_info(logger, "espacio de alloc totAL %d",
				 aux->nextAlloc - (desplazamiento + sizeof(HeapMetadata)));
		if (aux->esLibre && aux->nextAlloc - (desplazamiento + sizeof(HeapMetadata)) >= size)
		{

			return desplazamiento;
		}
		desplazamiento = aux->nextAlloc;

		if (tamanioPagina - (obtenerResto(aux->nextAlloc)) < 9)
		{
			log_info(logger, "HeapMetadata partido entre dos");
			aux = entreDosPaginas(aux->nextAlloc,proceso);
		}
		else
		{
			log_info(logger, "HeapMetadata normal");
			pagina = list_get(paginas,
							  obtenerDeterminadaPagina(desplazamiento));

			if (pagina->bitPresencia == false)
			{

				recuperacionPaginaFaltante(pid,
										   (uint16_t)obtenerDeterminadaPagina(desplazamiento));
			}
			log_info(logger,
					 " Datos Estado Pagina marco %d bitPrese %d bitUso %d",
					 pagina->marco, pagina->bitPresencia, pagina->bitUso);

			log_info(logger, "El desplazamiento es %d", desplazamiento);
			aux = memoriaCompleta + ((pagina->marco * tamanioPagina) + obtenerResto(desplazamiento));
		}
		log_info(logger, "HeapMetadata aux prev %d", aux->prevAlloc);
		log_info(logger, "HeapMetadata aux next %d", aux->nextAlloc);
		mostrarListaTLBs();
		mostrarPaginasTotales(paginas, pid);

		siguiente = aux->nextAlloc;
		log_info(logger, "El valor siguiente es %d", siguiente);
	}
	return -1;
}

void removerPaginaLRU(uint16_t marco, t_list *localLRU)
{
	bool buscarpaginaLRU(void *elem)
	{
		int *aAnalizar = elem;
		return *aAnalizar == marco;
	}
	if (string_equals_ignore_case(tipoAsignacion, "FIJA"))
	{
		list_remove_by_condition(localLRU, buscarpaginaLRU);
	}
	else
	{
		list_remove_by_condition(globalLRUCLOCK, buscarpaginaLRU);
	}
}
/*
uint32_t paginasAllocpresentes(uint32_t alloc, t_list *paginas, void *MMU)
{
	HeapMetadata *comienzo = MMU + (alloc - sizeof(HeapMetadata));
	uint32_t paginaFinal = obtenerDeterminadaPagina(comienzo->nextAlloc);
	for (int i = obtenerDeterminadaPagina(alloc); i <= paginaFinal; i++)
	{
		Pagina *pagina = list_get(paginas, i);
	}
}
*/
//----------------------------Remplazo de TLB carpincho------------------------------------
void nuevaEntradaTLB(uint16_t pagina, uint32_t marco, uint32_t id)
{
	TLB *entrada = malloc(sizeof(TLB));
	entrada->pagina = pagina;
	entrada->marco = marco;
	entrada->id = id;
	entrada->entrada = 1;
	list_add(registroTLB, entrada);
}
void mostrarListaTLBs()
{
	log_info(logger,
			 "====================Entradas TLB===================================");
	for (int i = 0; i < list_size(registroTLB); i++)
	{
		TLB *entrada = list_get(registroTLB, i);
		log_info(logger, "Marco: %d  Pagina: %d  IDCarpincho: %d",
				 entrada->marco, entrada->pagina, entrada->id);
	}
	log_info(logger,
			 "======================================================================");
}
void removerTLBvictima(uint16_t numeroPagina, uint32_t pid)
{
	bool removerReferenciaPagina(void *elem)
	{
		TLB *entrada = elem;
		return numeroPagina == entrada->pagina && entrada->id == pid;
	}
	list_remove_by_condition(registroTLB, removerReferenciaPagina);
}

TLB *devolberTLBconPagina(uint16_t pagina, uint32_t pid)
{
	bool buscarCoincidenciaTLB(void *tlb)
	{
		TLB *analizar = tlb;
		return pagina == analizar->pagina && analizar->id == pid;
	}
	TLB *encontrada;
	if (string_equals_ignore_case("FIFO", algoritmoRemplazoTLB))
	{
		encontrada = list_find(registroTLB, buscarCoincidenciaTLB);
		 return encontrada;
	}
	else if ((string_equals_ignore_case("LRU", algoritmoRemplazoTLB)))
	{
		encontrada = list_remove(registroTLB, buscarCoincidenciaTLB);
		list_add(registroTLB, encontrada);
		return encontrada;
	}
	return NULL;
}

void removerTodasLasEntradas(t_list *listaTLB)
{
	log_info(logger, "Todos los registros de entrada vacios");
	list_clean(listaTLB);
}
void removerTodasLasEntradasSuspencion(t_list *listaTLB, uint32_t id)
{
	bool buscarCoincidenciaTLBconID(void *tlb)
	{
		TLB *analizar = tlb;
		return id == analizar->id;
	}
	log_info(logger, "Todos los registros de entrada  con la ID vacios");
	TLB *verificar = list_remove_by_condition(registroTLB,
											  buscarCoincidenciaTLBconID);
	while (verificar != NULL)
	{
		verificar = list_remove_by_condition(registroTLB,
											 buscarCoincidenciaTLBconID);
	}
}
void aplicarAlgoritmoRemplazoTLB(uint32_t id, uint32_t cantidadRemplazos,
								 t_list *paginas, t_list *localLRU)
{

	for (int i = 1; i <= cantidadRemplazos; i++)
	{
		if (string_equals_ignore_case(algoritmoRemplazoTLB, "FIFO"))
		{
			log_info(logger, "ALGORITMO:El algoritmo a aplicar es FIFO");
			TLB *remplazo = buscarTLBvictimaFIFO(id);
			log_info(logger, "Se eligio una victima es la pagina %d marco %d",
					 remplazo->pagina, remplazo->marco);
			verificarEnvioPaginaRemplazoTLB(remplazo->pagina, remplazo->id,
											paginas);
			log_info(logger,
					 "Se envio para guardar el contenido de la pagina a remplazar");

			Marco *marco = list_get(listaDeMarcos, remplazo->marco);
			marco->esLibre = true;
		}
		else
		{
			log_info(logger, "ALGORITMO:El algoritmo a aplicar es LRU");
			TLB *remplazo = buscarTLBvictimaLRU(id);
			log_info(logger, "Se eligio una victima es la pagina %d marco %d",
					 remplazo->pagina, remplazo->marco);
			verificarEnvioPaginaRemplazoTLB(remplazo->pagina, remplazo->id,
											paginas);
			log_info(logger, "No es necesario guardar el contenido");
			Marco *marco = list_get(listaDeMarcos, remplazo->marco);
			marco->esLibre = true;
		}
	}
}
void aplicarAlgoritmoRemplazoTLBGlobal(uint32_t cantidadRemplazos)
{

	for (int i = 1; i <= cantidadRemplazos; i++)
	{
		if (string_equals_ignore_case(algoritmoRemplazoTLB, "FIFO"))
		{

			log_info(logger, "ALGORITMO:El algoritmo a aplicar es FIFO");

			TLB *remplazo = list_get(registroTLB, 0);
			log_info(logger, "Se eligio una victima es la pagina %d marco %d, id: %d",
					 remplazo->pagina, remplazo->marco, remplazo->id);
			AdmProceso *proceso = buscarCarpincho(remplazo->id);
			verificarEnvioPaginaRemplazoTLB(remplazo->pagina, remplazo->id,
											proceso->tablaDePaginas);
			log_info(logger,
					 "Se envio para guardar el contenido de la pagina a remplazar");

			Marco *marco = list_get(listaDeMarcos, remplazo->marco);
			marco->esLibre = true;
		}

		else
		{
			log_info(logger, "ALGORITMO:El algoritmo a aplicar es LRU");
			TLB *remplazo = list_remove(registroTLB, 0);
			log_info(logger, "Se eligio una victima es la pagina %d marco %d",
					 remplazo->pagina, remplazo->marco);
			AdmProceso *proceso = buscarCarpincho(remplazo->id);
			verificarEnvioPaginaRemplazoTLB(remplazo->pagina, remplazo->id,
											proceso->tablaDePaginas);

			Marco *marco = list_get(listaDeMarcos, remplazo->marco);
			marco->esLibre = true;
		}
		for (int i = 1; i <= cantidadRemplazos; i++)
		{
			if (string_equals_ignore_case(algoritmoRemplazoTLB, "FIFO"))
			{
				log_info(logger, "ALGORITMO:El algoritmo a aplicar es FIFO");
				TLB *remplazo = list_remove(registroTLB, 0);
				log_info(logger, "Se eligio una victima es la pagina %d marco %d, id: %d",
						 remplazo->pagina, remplazo->marco, remplazo->id);
				AdmProceso *proceso = buscarCarpincho(remplazo->id);
				verificarEnvioPaginaRemplazoTLB(remplazo->pagina, remplazo->id,
												proceso->tablaDePaginas);
				log_info(logger,
						 "Se envio para guardar el contenido de la pagina a remplazar");

				Marco *marco = list_get(listaDeMarcos, remplazo->marco);
				marco->esLibre = true;
			}

			else
			{
				log_info(logger, "ALGORITMO:El algoritmo a aplicar es LRU");
				TLB *remplazo = list_remove(registroTLB, 0);
				log_info(logger, "Se eligio una victima es la pagina %d marco %d",
						 remplazo->pagina, remplazo->marco);
				AdmProceso *proceso = buscarCarpincho(remplazo->id);
				verificarEnvioPaginaRemplazoTLB(remplazo->pagina, remplazo->id,
												proceso->tablaDePaginas);

				Marco *marco = list_get(listaDeMarcos, remplazo->marco);
				marco->esLibre = true;
			}
		}
	}
}
void verificarEnvioPaginaRemplazoTLB(uint16_t pagina, uint32_t id,
									 t_list *paginas)
{
	Pagina *aux = list_get(paginas, pagina);
	if (aux->bitModificacion)
	{
		log_info(logger, "Se envia la victima para sustitucion");
		void *aEnviar = malloc(tamanioPagina);
		memcpy(aEnviar, memoriaCompleta + (tamanioPagina * aux->marco),
			   tamanioPagina);

		enviarVictimaPageFault(id, pagina, aEnviar);

		aux->bitModificacion = false;
		aux->bitPresencia = false;
		aux->marco = -1;
	}
	else
	{
		log_info(logger, "No es necesario enviar la victima para sustitucion");
		aux->bitModificacion = false;
		aux->bitPresencia = false;
		aux->marco = -1;
	}
}
void verificarEnvioPaginaRemplazoTLBGlobal(uint16_t pagina, uint32_t id,
										   t_list *paginas)
{
	Pagina *aux = list_get(paginas, pagina);
	if (aux->bitModificacion)
	{
		log_info(logger, "Se envia la victima para sustitucion");
		void *aEnviar = malloc(tamanioPagina);
		memcpy(aEnviar, memoriaCompleta + (tamanioPagina * aux->marco),
			   tamanioPagina);

		enviarVictimaPageFault(id, pagina, aEnviar);

		aux->bitModificacion = false;
		aux->bitPresencia = false;
		aux->marco = -1;
	}
	else
	{
		log_info(logger, "No es necesario enviar la victima para sustitucion");
		aux->bitModificacion = false;
		aux->bitPresencia = false;
		aux->marco = -1;
	}
}
TLB *buscarTLBvictimaFIFO(uint32_t pid)
{
	for (int i = 0; i < list_size(registroTLB); i++)
	{
		TLB *tlb = list_get(registroTLB, i);
		if (tlb->id == pid)
		{
			return list_remove(registroTLB, i);
		}
	}
	return NULL;
}
TLB *buscarTLBvictimaLRU(uint32_t pid)
{
	TLB *victima = NULL;
	uint32_t posVictima;
	for (int i = 0; i < list_size(registroTLB); i++)
	{
		TLB *entrada = list_get(registroTLB, i);
		if (victima == NULL && pid == entrada->id)
		{
			victima = entrada;
			posVictima = i;
		}
		else if (pid == entrada->id)
		{
			if (victima->entrada < entrada->entrada)
			{
				victima = entrada;
				posVictima = i;
			}
		}
	}
	return list_remove(registroTLB, posVictima);
}

void *verificarPaginasAllocConTLBParaLectura(uint32_t id, uint32_t alloc, uint32_t size,
											 uint32_t *tamanioCont)
{
	TLB *encontrada = devolberTLBconPagina(obtenerDeterminadaPagina(alloc), id);
	
	void *contenido = malloc(size);
	AdmProceso *proceso = buscarCarpincho(id);
	if (encontrada == NULL)
	{
		return NULL;
		(*tamanioCont) = 0;
		TLBmissGlobal++;
		proceso->miss++;
	}
	else
	{
		log_info(logger,
				 "Se encontro la TLB buscada, proceder a verificar otras paginas del alloc");
		log_info(logger, "Datos TLB Pagina %d  Marco %d id %d", encontrada->pagina, encontrada->marco, encontrada->id);

		uint32_t desplazamiento = 0;
         void* ptrAlloc=memoriaCompleta + ((encontrada->marco * tamanioPagina) + obtenerResto(alloc));		TLBhitGlobal++;
		proceso->hit++;
		TLBhitGlobal++;

		uint32_t recorrido = 0;
		//En caso de que se quiera escribir contenido mayor al tamaño que puede contener

		if ((tamanioPagina - obtenerResto(alloc)) > size)
		{

			log_info(logger, "Lectura en la misma pagina");

			memcpy(contenido, ptrAlloc, size);
			return contenido;
		}
		else
		{
			memcpy(contenido, ptrAlloc,
				   tamanioPagina - obtenerResto(alloc));
			recorrido += tamanioPagina - obtenerResto(alloc);
		}
		uint16_t i = obtenerDeterminadaPagina(alloc) + 1;

		while (recorrido != size)
		{
			encontrada = devolberTLBconPagina(i, id);
			if (encontrada == NULL)
			{
				log_info(logger, "La TLB no esta presente en la lista");
				recuperacionPaginaFaltante(id, i);
				//Se recuperaron las paginas, se realizaron accesos a las paginas victima y solicitada para
				//Modificar sus datos
				encontrada = devolberTLBconPagina(i, id);
				log_info(logger,
						 "La pagina %d fue devuelta con exito de Swamp", i);
				TLBmissGlobal++;
				proceso->miss++;
			}
			else
			{
				TLBhitGlobal++;
				proceso->hit++;
			}
			log_info(logger, "Lectura en la pagina siguiente");
			if (tamanioPagina < (size - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(contenido,ptrAlloc , tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(contenido,ptrAlloc, (size - recorrido));
				recorrido += size - recorrido;
				mostrarListaTLBs();

				return contenido;
			}
		}
	}
}
bool escribirAllocConListaTLB(uint32_t id, uint32_t alloc,
							  uint32_t tamanioContenido, void *contenido)
{
	AdmProceso *proceso = buscarCarpincho(id);
	TLB *encontrada = devolberTLBconPagina(obtenerDeterminadaPagina(alloc), id);

	if (encontrada == NULL)

	{
		log_info(logger, "No se encontro la TLB, preguntar en paginas");
		proceso->miss++;
		TLBmissGlobal++;

		return false;
	}
	else
	{
		log_info(logger,
				 "Se encontro la TLB buscada, proceder a verificar otras paginas del alloc");
		log_info(logger, "DATA: MARCO %d, PAGINA: %d ID: %d", encontrada->marco, encontrada->pagina, encontrada->id);
		proceso->hit++;
		TLBhitGlobal++;

		log_info(logger, " El resto de la pagina es %d", obtenerResto(alloc));
		void *ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina) + obtenerResto(alloc));
	
		uint32_t recorrido = 0;
		//En caso de que se quiera escribir contenido mayor al tamaño que puede contener

		if ((tamanioPagina - obtenerResto(alloc)) > tamanioContenido)
		{

			log_info(logger, "Escritura en la misma pagina");

			memcpy(ptrAlloc, contenido, tamanioContenido);
			return true;
		}
		else
		{
			memcpy(ptrAlloc, contenido + recorrido,
				   tamanioPagina - obtenerResto(alloc));
			recorrido += tamanioPagina - obtenerResto(alloc);
		}
		uint16_t i = obtenerDeterminadaPagina(alloc) + 1;

		while (recorrido != tamanioContenido)
		{
			encontrada = devolberTLBconPagina(i, id);
			if (encontrada == NULL)
			{
				log_info(logger, "La TLB no esta presente en la lista");
				recuperacionPaginaFaltante(id, i);
				//Se recuperaron las paginas, se realizaron accesos a las paginas victima y solicitada para
				//Modificar sus datos
			
				log_info(logger,
						 "La pagina %d fue devuelta con exito de Swamp", i);
				TLBmissGlobal++;
				proceso->miss++;
						log_info(logger, "Escritura en la pagina siguiente");
			Pagina* devuelta=list_get(proceso->tablaDePaginas,i);
			if (tamanioPagina < (tamanioContenido - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((devuelta->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido, tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((devuelta->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido,
					   (tamanioContenido - recorrido));
				recorrido += tamanioContenido - recorrido;
				mostrarListaTLBs();

				return true;
			}
			}
			else
			{
				TLBhitGlobal++;
				proceso->hit++;
							log_info(logger, "Escritura en la pagina siguiente");
			if (tamanioPagina < (tamanioContenido - recorrido))
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido, tamanioPagina);
				recorrido += tamanioPagina;
			}
			else
			{
				ptrAlloc = memoriaCompleta + ((encontrada->marco * tamanioPagina));
				memcpy(ptrAlloc, contenido + recorrido,
					   (tamanioContenido - recorrido));
				recorrido += tamanioContenido - recorrido;
				mostrarListaTLBs();

				return true;
			}
			}
			i++;
mostrarListaTLBs();
		}
		

		return true;
	}
}
TLB *actualizarListaTLBporAlgoritmo(uint32_t marco)
{
	bool encontrarReferenciaMarco(void *aComparar)
	{
		TLB *elegida = aComparar;
		return elegida->marco == marco;
	}
	if (string_equals_ignore_case("LRU", algoritmoRemplazoTLB))
	{
		TLB *encontrada = list_remove_by_condition(registroTLB, encontrarReferenciaMarco);
		list_add(globalLRUCLOCK, encontrada);
		return encontrada;
	}
	else
	{
		TLB *encontrada = list_find(registroTLB, encontrarReferenciaMarco);
		return encontrada;
	}
}
bool verificarDuplicadoMarco(uint32_t marco)
{
	for (int i = 0; i < list_size(registroTLB); i++)
	{
		TLB *aAnalizar = list_get(registroTLB, i);
		if (marco == aAnalizar->marco)
		{
			return true;
		}
	}
	return false;
}
void controlarCreacionTLB(uint16_t pagina, uint32_t marco, uint32_t id)
{

	if (verificarDuplicadoMarco(marco))
	{
		log_info(logger, "El marco con la TLB ya existe");
	}
	else
	{
		if (cantidadEntradasTLB != list_size(registroTLB))
		{

			nuevaEntradaTLB(pagina, marco, id);
		}
		else
		{
			log_warning(logger, "Aplicando algoritmo de remplazo de TLB");
			TLB *aBorrar = list_remove(registroTLB, 0);
			nuevaEntradaTLB(pagina, marco, id);
			free(aBorrar);
		}
	}
}
/*
 void remplazarYenviar(uint32_t paginaRemplazo,uint32_t paginaNueva,t_list*paginas){
 Pagina* remplazo=list_get(paginas,paginaRemplazo);
 Pagina* nueva=list_get(paginas,paginaNueva);
 if(remplazo->bitModificacion){

 }else{

 }


 }
 */
//======================================Funciones par suspender proceso==============================
void realizarSuspencionProceso(uint32_t id, int socket)
{
	AdmProceso *carpincho = buscarCarpincho(id);
	uint32_t respuesta = true;
	mostrarListaTLBs();
	for (uint16_t i = 0; i < list_size(carpincho->tablaDePaginas); i++)
	{

		Pagina *aSuspender = list_get(carpincho->tablaDePaginas, i);
		if (aSuspender->bitPresencia)
		{
			if (aSuspender->bitModificacion)
			{
				void *aEnviar = malloc(tamanioPagina);
				memcpy(aEnviar,
					   memoriaCompleta + (aSuspender->marco * tamanioPagina),
					   tamanioPagina);
				HeapMetadata *ej = aEnviar;
				log_info(logger, "El HeapMetadata suspendido es HeapPrev %d",
						 ej->prevAlloc);
				log_info(logger, "El HeapMetadata suspendido es HeapNext %d",
						 ej->nextAlloc);
				enviarVictimaPageFault(id, i, aEnviar);
			}
			Marco *aLiberar = list_get(listaDeMarcos, aSuspender->marco);
			aLiberar->esLibre = true;
			aSuspender->bitPresencia = false;
			aSuspender->marco = -1;
			aSuspender->bitModificacion = false;
			aSuspender->bitUso = false;
		}
	}
	removerTodasLasEntradasSuspencion(registroTLB, id);
	mostrarPaginasTotales(carpincho->tablaDePaginas, carpincho->id_proceso);
	mostrarListaTLBs();

	send(socket, &respuesta, sizeof(uint32_t), 0);
}
//=================================================Eliminar Carpincho=====================================
AdmProceso *removerCarpincho(uint32_t id)
{
	bool buscarCarpinchoConID(void *elem)
	{
		AdmProceso *carpincho = elem;
		return carpincho->id_proceso == id;
	}
	return list_remove_by_condition(listaDeProcesos, buscarCarpinchoConID);
}

void liberarCarpinchoMemoria(uint32_t id)
{
	AdmProceso *carpincho = removerCarpincho(id);
	for (uint32_t i = 0; i < list_size(carpincho->tablaDePaginas); i++)
	{
		Pagina *pagina = list_remove(carpincho->tablaDePaginas, i);
		if (pagina->bitPresencia)
		{
			Marco *marco = list_get(listaDeMarcos, pagina->marco);
			marco->esLibre = true;
		}
		free(pagina);
	}
	uint8_t codigo = ELIMINARCARPINCHO;
	uint32_t sizeAEnviar = sizeof(uint8_t) + sizeof(uint32_t);
	void *aEnviar = malloc(sizeAEnviar);
	uint32_t desplazamiento = 0;
	memcpy(aEnviar, &(codigo), sizeof(uint8_t));
	desplazamiento = sizeof(uint8_t);
	memcpy(aEnviar + desplazamiento, &(id), sizeof(uint32_t));
	send(conexionSwamp, aEnviar, sizeAEnviar, 0);
	free(aEnviar);
	log_info(logger, "Se libero al carpincho %d, AUN JUGANDO: %d", id, list_size(listaDeProcesos));
}
