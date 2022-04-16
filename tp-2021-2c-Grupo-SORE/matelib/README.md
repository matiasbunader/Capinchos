# Interfaz para Biblioteca TP 2C2021 🧉

La propuesta de este repo es poder dejar delineada una interfaz de la
biblioteca planteada en el en el TP de Sistemas Operativos de la UTN FRBA
para el 2do cuatrimestre de 2021.

Por un lado, tenemos el archivo `matelib.h`, que es un archivo de Headers para
para usar, en el que cada equipo tiene que implementar la funcionalidad de
cada función definida allí.

## Conceptos

### ¿Qué es un archivo de Headers?
Hay muchas definiciones dando vueltas, su utilidad y detalles de su
implementación. Pero, en pocas palabras, es una buena forma de poner
definiciones y valores constantes que va a utilizar un programa en C.

Está bueno para definir `los prototipos` de las funciones (prototipo,
interfaz, interface, elija el nombre que más guste) de un módulo,
en este caso la biblioteca que van a usar los distintos procesos para
interactuar con el resto del sistema.

### ¿Una biblioteca?
La forma que tiene C (y la mayoría de los lenguajes de programación) para poder
compartir código entre distintos componentes, es crear una biblioteca.

Dado que en el TP los puntos de entrada son programas fuera de nuestro
control, la interacción de esos programas con el sistema va a ser mediante una
biblioteca que nos va a permitir:
- Planificar su ejecución.
- Gestionar memoria en un sistema centralizado.
- Hacer uso de recursos compartidos, como semáforos.

Pero **haciendo uso de lenguajes de programación existentes**, en vez de
algún lenguage propio que requiera un parser especial.

## Uso y funcionamiento esperado de la lib implementada
Antes de empezar a utilizar las funcionalidades de la lib, es necesario
inicializar las distintas estructuras administrativas que la misma va a
usar (como definiciones, conexiones, etc) mediante `mate_init`.

```c
void main() {
    mate_instance* lib;
    char* config_path = "...";
    int status = mate_init(lib, config_path);
    if(status != 0) {
        // Algo falló
    };
    // Ahora podemos usar lib como referencia.
};
```

Esta función recibe el path **relativo** del archivo de config a usar y un
puntero a la instancia de la "matelib" que vamos a usar para interactuar.
La especificación de la configuración será análoga a la de los otros
componentes del TP en forma, delegando al grupo la implementación para leer
el archivo e interpretar las distintas configuraciones.
**Como mínimo, el archivo de config debe contener**:
- IP de Conexión al Backend
- Puerto de Conexión al Backend

#### Backends
Un backend puede ser tanto un proceso Kernel, como un proceso Memoria.
Cada uno será identificado a la hora de realizar el hanshake de inicio,
permitiendo más o menos operaciones dependiendo de cual de los 2 sea.

En el caso del Kernel, se permitirán todas las operaciones, realizando un
"pasamanos" este último de los comandos al proceso Memoria.
Sin embargo, en el caso de la Memoria, solo los métodos de inicialización y
cierre; y de memoria, se encontrarán disponibles, *fallando el resto al tratar de usarse*.

*Es importante recalcar*, que en las operaciones de memoria, el grupo genere
la misma interfaz para tanto el módulo de Kernel, como para el de Memoria,
a fin de simplificar la implementación de la interacción.


#### mate_instance
La estructura de `mate_instance` en la especificación del TP se encuentra
vacía, con solo un puntero a algún tipo de estructura.
Es responsabilidad del grupo llenar dentro de la entidad todas las
referencias necesarias para mantener vivas las conexiones y estructuras
necesarias para operar con la misma.

El instancia será útil para referenciar distintos inits que se pueda hacer
en un mismo proceso.

Para identificar una instanciación, cada grupo deberá elegir alguna forma de
identificar cada una de estas instancias mediante algún medio (sean UUIDv4,
combinaciones de IPs, pids, números autogenerados, etc).


Como todo lo que se abre tiene que cerrarse, al finalizar de usar la lib, es
necesario llamar a `mate_close` usando la instancia de la lib como parámetro,
en el que la instancia de `mate_instance` deberá ser usada para librar los
recursos que referencia.

### Limitaciones de Implementación
Existen determinadas limitaciones y recomendaciones constructivas al usar
la lib en un proceso propio (y utilizables a la hora de testearla).

1. Esta lib puede no ser fork-safe, porque **se permiten File
Descriptors abiertos** y no se espera que se realice un manejo de los mismos
ante un escenario de fork.

> Sin embargo, cada grupo puede optar por evitar esta limitación mediante su
propia implementación. Pero no es un requerimiento obligatorio.

2. Esta lib debe soportar ser usada entre **varios threads**, cada uno con
realizando su propio `mate_init`. Es decir, la implementación de los grupos
**no puede hacer uso de estructuras y variables globales que puedan
generar conflictos entre varias llamadas a dicha función**.


## ¿Cómo hago para implementar la lib para mi TP?
En este repo, en la carpeta `examples/lib_implementation`, hay un ejemplo de
implementación de la lib.
Allí hacemos uso del `void*` de la estructura para usar una nuestra,
que manejaremos de forma interna con la información necearia para cada uno de
los módulos del TP.

Para este ejemplo, por simplicidad, solo va a responder las direcciones de
memoria `0`, el IO `PRINTER` y el semáforo `SEM1`, pero el espíritu es el mismo.

En la carpeta `examples/lib_usage` poseemos un ejemplo de programa
que hace uso de la lib que creamos y de las funcionalidades expuestas.

Para poder compilar todo, tenemos un Makefile que tiene 3 directivas, una para
compilar la lib, una para compilar al ejemplo que la usa, y finalmente, una
para correrlo (4 en realidad, con la de limpieza).

Para ver el ejemplo funcionar, ejecuta `make run`.


> **Nota**:
> Si revisan el ejemplo pueden ver como la referencia de la lib puede viajar
> entre hilos! Esto fue necesario porque solo implementamos el semáforo, de
> "forma local"; pero cada hilo podría hacer uso de su propio init!

## Requerimientos no funcionales
- Permitir un nivel de loggeo a modo de "debug", dado por config, de las
acciones de la biblioteca.
- Se deben utilizar las funciones de Kernel llamadas *bloqueantes*, para
poder planificar un proceso, no generando esperas activas.
- Las direcciones de memoria de la lib a utilizar, van a ser definidas como
int32_t (Enteros de 32bits con signo), para evitar problemas de tipado contra punteros locales.
