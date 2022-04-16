//#include "swap.h"
#include "servidor.h"

int main()
{
    iniciar_swamp();
    tipo_asignacion = ASIGNACION_GLOBAL;
    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    char *datos = "Esto es una muestra de datos a escribir";
    escribir_en_swap(13, 4, strlen(datos), datos);
    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    t_carpincho *carpincho = obtener_carpincho(13);
    for (int i = 0; i < list_size(carpincho->marcos); i++)
        leer_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));

    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    char *datos1 = "segunda muestra";
    escribir_en_swap(5, 4, strlen(datos1), datos1);
    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    carpincho = obtener_carpincho(5);
    for (int i = 0; i < list_size(carpincho->marcos); i++)
        leer_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));

    char *datos3 = "Tercer ejemplo";
    escribir_en_swap(95, 4, strlen(datos3), datos3);
    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    carpincho = obtener_carpincho(95);
    for (int i = 0; i < list_size(carpincho->marcos); i++)
        leer_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));

    char *datos4 = "el rodrigazo";
    escribir_en_swap(37, 4, strlen(datos4), datos4);
    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    carpincho = obtener_carpincho(37);
    for (int i = 0; i < list_size(carpincho->marcos); i++)
        leer_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));

    char *datos2 = "quintutela";
    escribir_en_swap(101, 4, strlen(datos2), datos2);
    printf("cantidad de carpinchos: %d\n", list_size(carpinchos));
    carpincho = obtener_carpincho(101);
    for (int i = 0; i < list_size(carpincho->marcos); i++)
        leer_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));

    for (int i = 0; i < list_size(carpinchos); i++)
    {
        t_carpincho *carpincho = list_get(carpinchos, i);
        printf("carpincho numero %d particion: %s\n", carpincho->id, carpincho->particion);
    }

    char *datos33 = "DYU";
    escribir_en_swap(13, 1, strlen(datos33), datos33);
    char *datos22 = "GOD";
    escribir_en_swap(101, 0, strlen(datos22), datos22);
    char *datos333 = "FIVE";
    escribir_en_swap(13, 5, strlen(datos333), datos333);

    escribir_en_swap(37, 3, strlen(datos333), datos333);
    escribir_en_swap(101, 7, strlen(datos333), datos333);
    escribir_en_swap(95, 4, strlen(datos333), datos333);
    for (int i = 0; i < list_size(carpinchos); i++)
    {
        t_carpincho *carpincho = list_get(carpinchos, i);
        printf("carpincho numero %d particion: %s\n", carpincho->id, carpincho->particion);
    }

    eliminar_carpincho(95);
    printf("el primer marco libre %d\n", obtener_primer_marco_libre("swap2.bin"));
    for (int i = 0; i < list_size(carpinchos); i++)
    {
        carpincho = list_get(carpinchos, i);
        printf("LECTURA FINAL CARPINCHO %d\n", carpincho->id);
        for (int i = 0; i < list_size(carpincho->marcos); i++)
            leer_marco_en_particion(carpincho->particion, *(uint32_t *)list_get(carpincho->marcos, i));
        //printf("carpincho numero %d, contenido marco 3: \"%d\" particion: %s\n", carpincho->id, *(int *)list_get(carpincho->marcos, 3), carpincho->particion);
    }

    for (int i = 0; i < list_size(carpinchos); i++)
    {
        t_carpincho *carpincho = list_get(carpinchos, i);
        printf("carpincho numero %d particion: %s\n", carpincho->id, carpincho->particion);
    }
    /*
    int server_fd = levantar_swamp();
    int memoria_fd = esperar_cliente(server_fd, logger);
    con_param params;
    params.logger = logger;
    params.conexion = memoria_fd;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t tid1;
    pthread_create(&tid1, &attr, (void *)hablar, &params);

    pthread_t tid2;
    pthread_create(&tid2, &attr, (void *)escuchar, &params);

  
    char particion[20];
    uint32_t maximo = maximos_marcos_libres(particion);
    printf("Maximos marcos libres: %d, en particion %s, numero de particion: %d\n", maximo, particion, obtener_numero_particion(particion));

    escribir_marco_en_particion(particion, "yo yamil", 0);
    escribir_marco_en_particion(particion, "esto es una frase mas", 30);
    escribir_marco_en_particion(particion, "yo yamil", 11);
    printf("marcos libres: %d\n", marcos_libres(particion));
    printf("marco 0: %s\n", (char *)leer_marco_en_particion(particion, 0));
    printf("marco 11: %s\n", (char *)leer_marco_en_particion(particion, 0));
    printf("marco 30: %s\n", (char *)leer_marco_en_particion(particion, 30));
    eliminar_marco_en_particion(particion, 30);
    eliminar_marco_en_particion(particion, 0);
    eliminar_marco_en_particion(particion, 11);
    printf("marcos libres: %d\n", marcos_libres(particion));
    printf("marco 0: %s\n", (char *)leer_marco_en_particion(particion, 0));
    printf("marco 11: %s\n", (char *)leer_marco_en_particion(particion, 0));
    printf("marco 30: %s\n", (char *)leer_marco_en_particion(particion, 30));
*/
    limpiar_swamp();

    /*
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
 
 */
    return 0;
}