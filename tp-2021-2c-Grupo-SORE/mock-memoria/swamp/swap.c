#include "swap.h"

struct
{
    uint32_t cantidad_marcos; //marcos por particion
    uint32_t tamanio_swap;
    uint32_t tamanio_pagina;
    uint16_t marcos_maximos; //marcos maximos por carpincho, solo aplicable en asignacion fija
    char **rutas;
    uint16_t retardo;
} swap_info;

void iniciar_swamp()
{
    logger = log_create("swamp.log", "Swamp", 1, LOG_LEVEL_DEBUG);
    t_config *meta = config_create(ARCHIVO_CONFIG);
    swap_info.tamanio_pagina = obtener_tamanio_pagina(meta);
    swap_info.tamanio_swap = obtener_tamanio_swap(meta);
    swap_info.retardo = obtener_retardo(meta);
    swap_info.marcos_maximos = obtener_marcos_por_carpincho(meta);
    void *contenido_inicial_swap = malloc(swap_info.tamanio_swap);
    memset(contenido_inicial_swap, '\0', swap_info.tamanio_swap);
    //uint32_t cantidad_marcos = marcos_totales();
    swap_info.cantidad_marcos = marcos_totales();
    log_info(logger, "CANTIDAD DE MARCOS POR PARTICION: %d\n", swap_info.cantidad_marcos);
    carpinchos = list_create();
    tipo_asignacion = ASIGNACION_FIJA;
    //leo las rutas de los swap
    //char **rutas_swap = config_get_array_value(meta, "ARCHIVOS_SWAP");
    swap_info.rutas = config_get_array_value(meta, "ARCHIVOS_SWAP");
    int cantidad_swap = 0;
    for (; swap_info.rutas[++cantidad_swap] != NULL;) //Recorro los elementos para contar la cantidad de particiones swap
        ;
    log_info(logger, "INICIAR_SWAP: CANTIDAD DE PARTICIONES SWAP: %d\n", cantidad_swap);

    for (int i = 0; i < cantidad_swap; i++)
    {
        char **aux = string_split(swap_info.rutas[i], "/");
        int cant_aux = 0;
        for (; aux[++cant_aux] != NULL;)
            ;
        char nombre_particion_swap[strlen(aux[cant_aux - 1])];
        strcpy(nombre_particion_swap, aux[cant_aux - 1]);

        printf("ruta: %s\n", swap_info.rutas[i]);

        FILE *swap = fopen(swap_info.rutas[i], "w");
        if (!swap) //Verifico que swap haya abierto correctamente(swap = NULL), sino lanzo error
            log_error(logger, "INICIAR_SWAP: error en la creacion/apertura de particion de swap %s", nombre_particion_swap);
        int escrito = fwrite(contenido_inicial_swap, 1, swap_info.tamanio_swap, swap);

        if (escrito < swap_info.tamanio_swap)
            log_warning(logger, "INICIAR_SWAP: Cuidado: Posible error en la creacion del swap[%s], verifique tamanio\ncantidad de escrituras: %d y tamanio de swap: %d\n", nombre_particion_swap, escrito, swap_info.tamanio_swap);
        else
            log_warning(logger, "INICIAR_SWAP: Swap[%s] creado\nCantidad de escrituras: %d\n", nombre_particion_swap, escrito);
        crear_bitmap(nombre_particion_swap, swap_info.cantidad_marcos);
        fclose(swap);
    }
    free(contenido_inicial_swap);
    config_destroy(meta);
}

t_log *obtener_logger()
{
    return logger;
}

void crear_bitmap(char *nombre_swap, int cantidad_marcos)
{
    int16_t *bitchar = calloc(1, cantidad_marcos / 8);
    //t_bitarray *bitarray = bitarray_create_with_mode(bitchar,cantidad_marcos/8,LSB_FIRST);
    char nombre_bitmap[75];
    sprintf(nombre_bitmap, "%s/bitmap_%s", RUTA_BITMAP, nombre_swap);
    FILE *bitmap = fopen(nombre_bitmap, "w");
    int escrito = fwrite(bitchar, 1, cantidad_marcos / 8, bitmap);
    if (escrito < cantidad_marcos / 8)
        log_warning(logger, "Posible error en la creacion del bitmap %s, verifique tamanio\ncantidad de escrituras: %d\n", nombre_bitmap, escrito);
    else
        log_info(logger, "Bitmap creado\nCantidad de escrituras: %d\n", escrito);

    free(bitchar);
    fclose(bitmap);
}

t_bitarray *obtener_bitmap(char *particion)
{
    int tamanio = marcos_totales() / 8;
    char *buf = malloc(tamanio);
    t_bitarray *bitarray = bitarray_create_with_mode(buf, tamanio, LSB_FIRST);
    char nombre_bitmap[50];
    sprintf(nombre_bitmap, "%s/bitmap_%s", RUTA_BITMAP, particion);
    FILE *bitmap = fopen(nombre_bitmap, "r");
    fread(bitarray->bitarray, 1, tamanio, bitmap);
    fclose(bitmap);
    //free(buf); //Debo liberar buf?
    return bitarray;
}

//devuelve la cantidad de marcos totales por particion swap
uint32_t marcos_totales()
{

    /*t_config *meta = config_create(ARCHIVO_CONFIG);
    uint32_t tamanio_pagina = config_get_int_value(meta, "TAMANIO_PAGINA");
    uint32_t tamanio_swap = config_get_int_value(meta, "TAMANIO_SWAP");
    config_destroy(meta);
    */
    if (swap_info.tamanio_pagina == 0)
        return 0;
    else
        return swap_info.tamanio_swap / swap_info.tamanio_pagina;
}

int marcos_libres(char *particion)
{
    t_bitarray *bitarray = obtener_bitmap(particion);
    int marcos = (int)bitarray_get_max_bit(bitarray);
    int marcos_libres = marcos;
    for (int i = 0; i < marcos; i++)
        if (bitarray_test_bit(bitarray, i))
            marcos_libres--;
    bitarray_destroy(bitarray);
    return marcos_libres;
}

//Devuelve la maxima cantidad de marcos libres comparando todas las particiones existentes, copia el nombre de la particion resultante en el argumento
uint32_t maximos_marcos_libres(char *particion)
{
    int cantidad_swap = 0;
    uint32_t maximo_libre = 0;
    for (; swap_info.rutas[++cantidad_swap] != NULL;) //para loopear entre los elementos para crear todos los swaps
        ;

    for (int i = 0; i < cantidad_swap; i++)
    {
        char **aux = string_split(swap_info.rutas[i], "/");
        int cant_aux = 0;
        for (; aux[++cant_aux] != NULL;)
            ;
        char nombre_particion_swap[strlen(aux[cant_aux - 1])];
        strcpy(nombre_particion_swap, aux[cant_aux - 1]);
        uint32_t aux_max = marcos_libres(nombre_particion_swap);
        if (aux_max > maximo_libre)
        {
            maximo_libre = aux_max;
            strcpy(particion, nombre_particion_swap);
        }
    }
    return maximo_libre;
}

int32_t obtener_primer_marco_libre(char *particion)
{
    t_bitarray *bitarray = obtener_bitmap(particion);

    int nfree = -1;
    for (int i = 0; i < bitarray->size * 8; i++)
        if (!bitarray_test_bit(bitarray, i))
        {
            nfree = i; //return i; ??
            break;
        }
    bitarray_destroy(bitarray);
    return nfree;
}

uint32_t obtener_numero_particion(char *particion)
{
    return atoi(&particion[strlen(particion) - 5]);
}

//pre-condicion: las particiones vienen listadas en orden numerico(base 1) en el archivo de configuracion, sin huecos ej:../swap1.bin, ../swap2.bin,...
void escribir_marco_en_particion(char *particion, void *datos, uint32_t marco)
{
    t_bitarray *bitarray = obtener_bitmap(particion);
    int tamanio = (int)bitarray_get_max_bit(bitarray);
    uint32_t numero_particion = obtener_numero_particion(particion);
    //char **rutas_swap = config_get_array_value(meta, "ARCHIVOS_SWAP");
    FILE *archivo_particion = fopen(swap_info.rutas[numero_particion - 1], "r+");
    if (tamanio < marco)
        log_error(logger, "ESCRIBIR_MARCO_EN_PARTICION: NUMERO DE MARCOS (%d) MENOR AL SOLICITADO (%d)", tamanio, marco);
    else if (fseek(archivo_particion, marco * swap_info.tamanio_pagina, SEEK_SET) == 0)
    {
        if (fwrite(datos, 1, strlen(datos), archivo_particion) == strlen(datos))
        {
            log_info(logger, "ESCRIBIR_MARCO_EN_PARTICION: Se escribio \"%s\", en el marco %d de la particion %s", datos, marco, particion);
            bitarray_set_bit(bitarray, marco);
            actualizar_bitmap(particion, bitarray);
            if (strlen(datos) < swap_info.tamanio_pagina)
            {
                void *contenido_restante = malloc(swap_info.tamanio_pagina - strlen(datos));
                memset(contenido_restante, '\0', swap_info.tamanio_pagina - strlen(datos));
                fwrite(contenido_restante, 1, swap_info.tamanio_pagina - strlen(datos), archivo_particion); //relleno el resto del marco con '\0'
                free(contenido_restante);
            }
        }

        else
            log_error(logger, "ESCRIBIR_MARCO_EN_PARTICION: Error en la escritura de %s en el marco %d de la particion %s\n", datos, marco, particion);
    }
    else
        log_error(logger, "ESCRIBIR_MARCO_EN_PARTICION: Error en el posicionamiento del marco %d en la particion %s\n", marco, particion);
    bitarray_destroy(bitarray);
    fclose(archivo_particion);
}
//actualiza el bitmap de la particion con el bitarray
void actualizar_bitmap(char *particion, t_bitarray *bitarray)
{
    char ruta_bitmap[60];
    sprintf(ruta_bitmap, "%s/bitmap_%s", RUTA_BITMAP, particion);
    FILE *bitmap = fopen(ruta_bitmap, "r+");
    if (bitmap == NULL)
        log_error(logger, "ACTUALIZAR_BITMAP: ERROR EN LA ESCRITURA DEL BITMAP DE LA PARTICION %s", particion);
    else if (fwrite(bitarray->bitarray, 1, bitarray->size, bitmap) != bitarray->size)
        log_error(logger, "ACTUALIZAR_BITMAP: TAMAÑO DE DATOS ACTUALIZADOS EN BITMAP DIFERENTE AL DEL BITARRAY");
    fclose(bitmap);
}
//Retorna el tamaño de la página o 0 en caso de error
uint32_t obtener_tamanio_pagina(t_config *meta)
{
    uint32_t tamanio_pagina = 0;
    if (config_has_property(meta, "TAMANIO_PAGINA"))
        tamanio_pagina = config_get_int_value(meta, "TAMANIO_PAGINA");
    else
        log_error(logger, "OBTENER_TAMANIO_PAGINA: No se encontro el valor TAMANIO_PAGINA\n");
    return tamanio_pagina;
}

//Retorna la cantidad maxima de particiones por carpincho -solo aplicable cuando hay tipo de asignacion fija-
uint16_t obtener_marcos_por_carpincho(t_config *meta)
{
    uint16_t marcos_por_carpincho = 0;
    if (config_has_property(meta, "MARCOS_POR_CARPINCHO"))
        marcos_por_carpincho = (uint16_t)(config_get_int_value(meta, "MARCOS_POR_CARPINCHO"));
    else
        log_error(logger, "OBTENER_MARCOS_POR_CARPINCHO: No se encontro el valor MARCOS_POR_CARPINCHO\n");
    log_info(logger, "OBTENER_MARCOS_POR_CARPINCHO: La cantidad de marcos por carpincho es %d", marcos_por_carpincho);
    return marcos_por_carpincho;
}

//Retorna el tiempo de retardo
uint16_t obtener_retardo(t_config *meta)
{
    uint16_t tiempo_retardo = 0;
    if (config_has_property(meta, "RETARDO_SWAP"))
        tiempo_retardo = (uint16_t)(config_get_int_value(meta, "RETARDO_SWAP"));
    else
        log_error(logger, "No se encontro el valor RETARDO_SWAP\n");
    log_info(logger, "El tiempo de retardo seteado es %d", tiempo_retardo);
    return tiempo_retardo;
}

void retardo()
{
    if (usleep(swap_info.retardo * 1000) == -1)
        log_warning(logger, "RETARDO: el tiempo de retardo no se cumplimento, error: %d", errno);
}

uint16_t cantidad_marcos_por_escribir(uint32_t tamanio_datos)
{
    uint16_t marcos_por_escribir = 1;
    if (tamanio_datos > swap_info.tamanio_pagina)
    {
        marcos_por_escribir = tamanio_datos / swap_info.tamanio_pagina;
        if ((tamanio_datos % swap_info.tamanio_pagina) != 0)
            marcos_por_escribir++;
    }
    return marcos_por_escribir;
}

//Retorna el tamaño de la partición swap o 0 en caso de error
uint32_t obtener_tamanio_swap(t_config *meta)
{
    uint32_t tamanio_swap = 0;
    if (config_has_property(meta, "TAMANIO_SWAP"))
        tamanio_swap = atoi(config_get_string_value(meta, "TAMANIO_SWAP"));
    else
        log_error(logger, "No se encontro el valor TAMANIO_SWAP\n");
    return tamanio_swap;
}

//pre-condicion: las particiones vienen listadas en orden numerico(base 1) en el archivo de configuracion, sin huecos ej:[../swap1.bin, ../swap2.bin,...]
void *leer_marco_en_particion(char *particion, uint32_t marco)
{
    void *datos = malloc(swap_info.tamanio_pagina);
    uint8_t numero_particion = obtener_numero_particion(particion);
    //char **rutas_swap = config_get_array_value(meta, "ARCHIVOS_SWAP");
    FILE *archivo_particion = fopen(swap_info.rutas[numero_particion - 1], "r");

    if (fseek(archivo_particion, marco * swap_info.tamanio_pagina, SEEK_SET) == 0)
    {
        if (fread(datos, 1, swap_info.tamanio_pagina, archivo_particion) == swap_info.tamanio_pagina)
        {
            log_info(logger, "LEER_MARCO_EN_PARTICION: Se leyo \"%s\", en el marco %d de la particion %s\n", datos, marco, particion);
        }

        else
            log_error(logger, "LEER_MARCO_EN_PARTICION: Error en la lectura de \"%s\" en el marco %d de la particion %s\n", datos, marco, particion);
    }
    else
        log_error(logger, "LEER_MARCO_EN_PARTICION: Error en el posicionamiento del marco %d en la particion %s\n", marco, particion);
    //config_destroy(meta);
    fclose(archivo_particion);
    return datos;
}

//pre-condicion: las particiones vienen listadas en orden numerico(base 1) en el archivo de configuracion, sin huecos ej:../swap1.bin, ../swap2.bin,...
void eliminar_marco_en_particion(char *particion, uint32_t marco)
{
    t_bitarray *bitarray = obtener_bitmap(particion);
    void *contenido_marco_vacio = malloc(swap_info.tamanio_pagina);
    memset(contenido_marco_vacio, '\0', swap_info.tamanio_pagina);
    escribir_marco_en_particion(particion, contenido_marco_vacio, marco);
    bitarray_clean_bit(bitarray, marco);
    actualizar_bitmap(particion, bitarray);
    bitarray_destroy(bitarray);
    free(contenido_marco_vacio);
}

void limpiar_swamp()
{
    int cantidad_swap = 0;
    for (; swap_info.rutas[++cantidad_swap] != NULL;) //Recorro los elementos para contar la cantidad de particiones swap
        ;
    for (int i = 0; i < cantidad_swap; i++)
    {
        char **aux = string_split(swap_info.rutas[i], "/");
        int cant_aux = 0;
        for (; aux[++cant_aux] != NULL;)
            ;
        char nombre_particion_swap[strlen(aux[cant_aux - 1])];
        strcpy(nombre_particion_swap, aux[cant_aux - 1]);
        char nombre_bitmap[75];
        sprintf(nombre_bitmap, "%s/bitmap_%s", RUTA_BITMAP, nombre_particion_swap);

        if (unlink(nombre_bitmap) == 0)
            log_info(logger, "LIMPIAR_SWAMP: BITMAP %s ELIMINADO", nombre_bitmap);
        else
            log_error(logger, "LIMPIAR_SWAMP: NO SE PUDO ELIMINAR BITMAP %s", nombre_bitmap);

        if (unlink(swap_info.rutas[i]) == 0)
            log_info(logger, "LIMPIAR_SWAMP: PARTICION SWAP %s ELIMINADO", swap_info.rutas[i]);
        else
            log_error(logger, "LIMPIAR_SWAMP: NO SE PUDO ELIMINAR PARTICION SWAP %s", swap_info.rutas[i]);
    }
    log_destroy(logger);
}

void escribir_en_swap(uint32_t carpincho, uint16_t pagina, uint32_t tamanio_datos, void *datos)
{
    if (tipo_asignacion == ASIGNACION_FIJA)
        escribir_por_asignacion_fija(carpincho, pagina, tamanio_datos, datos);
    else
        escribir_por_asignacion_global(carpincho, pagina, tamanio_datos, datos);
}

void mover_carpincho_a_particion(t_carpincho *carpincho, char *particion)
{
    uint16_t marcos = list_size(carpincho->marcos);
    if (marcos > marcos_libres(particion))
    {
        log_error(logger, "MOVER_CARPINCHO_A_PARTICION: NO SE ENCONTRARON MARCOS LIBRES EN PARTICION [%s] DESTINO, NO SE MOVIO CARPINCHO %d", particion, carpincho->id);
        return;
    }
    for (int i = 0; i < marcos; i++)
    {
        uint32_t *marco_destino = malloc(sizeof(uint32_t));
        *marco_destino = obtener_primer_marco_libre(particion);
        void *pagina_aux = malloc(swap_info.tamanio_pagina);
        int marco = *(int *)list_get(carpincho->marcos, i);
        strncpy(pagina_aux, leer_marco_en_particion(carpincho->particion, marco), swap_info.tamanio_pagina);
        escribir_marco_en_particion(particion, pagina_aux, *marco_destino);
        eliminar_marco_en_particion(carpincho->particion, marco);
        void *marco_viejo = list_replace(carpincho->marcos, i, marco_destino);
        free(marco_viejo);
    }
    log_info(logger, "MOVER_CARPINCHO_A_PARTICION: se elimina carpincho con id %d de particion %s", carpincho->id, carpincho->particion);
    log_info(logger, "MOVER_CARPINCHO_A_PARTICION: se movio carpincho con id %d a particion %s", carpincho->id, carpincho->particion);
    strcpy(carpincho->particion, particion);
}

void escribir_por_asignacion_fija(uint32_t carpincho_id, uint16_t pagina, uint32_t tamanio_datos, void *datos)
{
    t_carpincho *carpincho = obtener_carpincho(carpincho_id);
    uint16_t marcos = list_size(carpincho->marcos);
    char particion[20];
    if (marcos == 0) //inicializo el carpincho, caso en que es nuevo
    {
        maximos_marcos_libres(particion);
        strcpy(carpincho->particion, particion);
        int marco;
        if ((marco = obtener_primer_marco_libre(particion)) == -1)
        {
            log_error(logger, "ESCRIBIR_POR_ASIGNACION_FIJA: NO SE ENCONTRARON MARCOS LIBRES");
            return;
        }
        //reservo los marcos requeridos para el nuevo carpincho
        for (int i = 0; i < swap_info.marcos_maximos; i++)
            agregar_marco_vacio_carpincho(carpincho);

        log_info(logger, "ESCRIBIR_POR_ASIGNACION_FIJA: se creo nuevo carpincho con id %d en particion %s", carpincho->id, carpincho->particion);
        //escribo los datos en el carpincho nuevo, pueden ser mas de una pagina/marco
        escribir_marco_en_particion(carpincho->particion, datos, *(uint32_t *)list_get(carpincho->marcos, 0));
    }
    else
    {
        escribir_marco_en_particion(carpincho->particion, datos, *(uint32_t *)list_get(carpincho->marcos, pagina));
        log_info(logger, "ESCRIBIR_POR_ASIGNACION_FIJA: se escribio pagina %d de carpincho con id %d en la particion %s", pagina, carpincho->id, carpincho->particion);
    }
}

//devuelve un puntero al carpincho correspondiente al id. Si no existe, lo crea, agrega a la lista y retorna el puntero al mismo
t_carpincho *obtener_carpincho(uint32_t carpincho_id)
{
    int cantidad_carpinchos = list_size(carpinchos);
    //t_carpincho *carpincho = malloc(sizeof(t_carpincho));
    for (int i = 0; i < cantidad_carpinchos; i++)
    {
        t_carpincho *carpincho = list_get(carpinchos, i);
        if (carpincho->id == carpincho_id)
            return carpincho;
    }
    t_carpincho *carpincho = malloc(sizeof(t_carpincho));
    carpincho->id = carpincho_id;
    carpincho->marcos = list_create();
    carpincho->particion = malloc(20);
    list_add(carpinchos, carpincho);
    return carpincho;
}

//Agrega marco vacio a carpincho devolviendo el numero de marco asignado, en caso de error devuelve -1
int agregar_marco_vacio_carpincho(t_carpincho *carpincho)
{
    int *aux = malloc(sizeof(int));
    if ((*aux = obtener_primer_marco_libre(carpincho->particion)) == -1)
    {
        log_error(logger, "AGREGAR_MARCO_VACIO_CARPINCHO: NO SE ENCONTRARON MARCOS LIBRES, NO SE SWAPPEO CARPINCHO %d", carpincho->id);
        return *aux;
    }
    list_add(carpincho->marcos, aux);
    escribir_marco_en_particion(carpincho->particion, "", *aux);
    return *aux;
}

void escribir_por_asignacion_global(uint32_t carpincho_id, uint16_t pagina, uint32_t tamanio_datos, void *datos)
{
    t_carpincho *carpincho = obtener_carpincho(carpincho_id);
    uint16_t marcos = list_size(carpincho->marcos);
    char particion[20];
    if (marcos == 0) //inicializo el carpincho, caso en que es nuevo
    {
        uint32_t marcos_libres_en_particion = maximos_marcos_libres(particion);
        strcpy(carpincho->particion, particion);
        uint16_t cantidad_marcos = cantidad_marcos_por_escribir(tamanio_datos);
        if (cantidad_marcos > marcos_libres_en_particion)
        {
            log_error(logger, "ESCRIBIR_POR_ASIGNACION_GLOBAL: NO SE ENCONTRARON SUFICIENTES MARCOS LIBRES, NO SE SWAPPEO CARPINCHO %d", carpincho_id);
            return;
        }
        //reservo los marcos requeridos para el nuevo carpincho
        for (int i = 0; i < cantidad_marcos; i++)
            agregar_marco_vacio_carpincho(carpincho);
        log_info(logger, "ESCRIBIR_POR_ASIGNACION_GLOBAL: se creo nuevo carpincho con id %d en particion %s", carpincho->id, carpincho->particion);
        //escribo los datos en el carpincho nuevo, pueden ser mas de una pagina/marco
        for (int i = 0; i < cantidad_marcos; i++)
        {
            void *datos_aux = malloc(swap_info.tamanio_pagina);
            strncpy(datos_aux, datos + i * swap_info.tamanio_pagina, swap_info.tamanio_pagina);
            escribir_marco_en_particion(carpincho->particion, datos_aux, *(uint32_t *)list_get(carpincho->marcos, i));
        }
    }
    else if (pagina == marcos)
    {
        int marco;
        if ((marco = agregar_marco_vacio_carpincho(carpincho)) == -1)
            log_error(logger, "ESCRIBIR_POR_ASIGNACION_GLOBAL: NO SE ENCONTRARON MARCOS LIBRES EN LA PARTICION %s PARA ASIGNAR AL CARPINCHO %d", carpincho->particion, carpincho->id);
        else
            escribir_marco_en_particion(carpincho->particion, datos, marco);
    }
    else if (pagina < marcos) //strcmp(carpincho->particion, particion) == 0) //la particion en la que se encuentra es la que mas espacio libre tiene, solo escribo una pagina
    {
        escribir_marco_en_particion(carpincho->particion, datos, *(uint32_t *)list_get(carpincho->marcos, pagina));
        log_info(logger, "ESCRIBIR_POR_ASIGNACION_GLOBAL: se escribio pagina %d de carpincho con id %d a particion %s", pagina, carpincho->id, carpincho->particion);
    }
    else
        log_error(logger, "ESCRIBIR_POR_ASIGNACION_GLOBAL: OPERACION INVALIDA, SE INTENTA ESCRIBIR EN PAGINA %d CUANDO EL NUMERO DE PAGINA MAXIMO ACTUAL ES %d", pagina, list_size(carpincho->marcos) - 1);
}

bool esta_carpincho(uint32_t carpincho_id)
{
    int cantidad_carpinchos = list_size(carpinchos);
    for (int i = 0; i < cantidad_carpinchos; i++)
    {
        t_carpincho *carpincho = list_get(carpinchos, i);
        if (carpincho->id == carpincho_id)
            return true;
    }
    return false;
}

void eliminar_carpincho(uint32_t carpincho_id)
{
    if (!esta_carpincho(carpincho_id))
    {
        log_error(logger, "ELIMINAR_CARPINCHO: EL CARPINCHO %d NO EXISTE EN SWAMP.", carpincho_id);
        return;
    }
    t_carpincho *carpincho = malloc(sizeof(t_carpincho));
    int cantidad_carpinchos = list_size(carpinchos);
    for (int i = 0; i < cantidad_carpinchos; i++)
    {
        carpincho = list_get(carpinchos, i);
        if (carpincho->id == carpincho_id)
        {
            carpincho = list_replace(carpinchos, i, list_get(carpinchos, cantidad_carpinchos - 1));
            break;
        }
    }
    int marcos = list_size(carpincho->marcos);
    for (int i = marcos - 1; i >= 0; i--)
    {
        eliminar_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));
        free(list_get(carpincho->marcos, i));
    }
    list_destroy(carpincho->marcos);
    free(carpincho->particion);
    free(carpincho);
    list_remove(carpinchos, cantidad_carpinchos - 1);
    log_info(logger, "ELIMINAR_CARPINCHO: SE ELIMINO CARPINCHO %d.", carpincho_id);
}