#ifndef SWAP_H
#define SWAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#define ARCHIVO_CONFIG "swamp.config"
#define RUTA_BITMAP "."

typedef struct
{
    int id;
    char *particion;
    t_list *marcos;
} t_carpincho;

typedef enum
{
    ASIGNACION_FIJA = 1,
    ASIGNACION_GLOBAL
} t_asignacion;

t_list *carpinchos;

t_log *logger;

t_asignacion tipo_asignacion;

void iniciar_swamp();
t_log *obtener_logger();
void crear_bitmap(char *, int);
t_bitarray *obtener_bitmap(char *);
int marcos_libres(char *);
uint32_t marcos_totales();
uint32_t maximos_marcos_libres(char *);
int32_t obtener_primer_marco_libre(char *);
void escribir_marco_en_particion(char *, void *, uint32_t);
uint32_t obtener_numero_particion(char *);
void actualizar_bitmap(char *, t_bitarray *);
uint32_t obtener_tamanio_pagina(t_config *);
uint32_t obtener_tamanio_swap(t_config *);
uint16_t obtener_retardo(t_config *);
void *leer_marco_en_particion(char *, uint32_t);
void eliminar_marco_en_particion(char *, uint32_t);
int agregar_marco_vacio_carpincho(t_carpincho *);
void mover_carpincho_a_particion(t_carpincho *, char *);
void limpiar_swamp();
uint16_t cantidad_marcos_por_escribir(uint32_t);
uint16_t obtener_marcos_por_carpincho(t_config *);
void retardo();
void escribir_en_swap(uint32_t, uint16_t, uint32_t, void *);
void escribir_por_asignacion_fija(uint32_t, uint16_t, uint32_t, void *);
void escribir_por_asignacion_global(uint32_t, uint16_t, uint32_t, void *);
t_carpincho *obtener_carpincho(uint32_t);
bool esta_carpincho(uint32_t);
void eliminar_carpincho(uint32_t);

#endif