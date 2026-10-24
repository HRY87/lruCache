# Trabajo Práctico: LRU Cache

Implementación en **C** de un TDA **LRU Cache** (*Least Recently Used*) en dos versiones, más una simulación de la red social ficticia **Tweener** que mide cuánto mejora (o no) el procesamiento al usar la caché.

| | |
|---|---|
| Lenguaje | C (C99, solo biblioteca estándar) |
| IDE | Code::Blocks 20.03 a 25.03 (proyecto `lruCache.cbp`) |
| Compilador | GCC (MinGW en Windows, gcc en Linux/macOS) |

---

## Índice

1. [Qué es una LRU Cache](#1-qué-es-una-lru-cache)
2. [Mapa del proyecto](#2-mapa-del-proyecto)
3. [Compilar y ejecutar](#3-compilar-y-ejecutar)
4. [Base común: `utilidades` y formato de entrada `[clave][dato]`](#4-base-común-utilidades-y-formato-de-entrada-clavedato)
5. [PARTE 1: caché con lista simple](#5-parte-1-caché-con-lista-simple)
6. [PARTE 2: caché con lista doble + árbol índice](#6-parte-2-caché-con-lista-doble--árbol-índice)
7. [Manejo de memoria (Partes 1 y 2)](#7-manejo-de-memoria-partes-1-y-2)
8. [PARTE 3: simulación Tweener](#8-parte-3-simulación-tweener)
9. [Ingreso de datos: qué pasa con cada entrada](#9-ingreso-de-datos-qué-pasa-con-cada-entrada)
10. [Test cruzado](#10-test-cruzado)
11. [Resultados esperados y ejemplo](#11-resultados-esperados-y-ejemplo)
12. [Decisiones de diseño y limitaciones](#12-decisiones-de-diseño-y-limitaciones)

---

## 1. Qué es una LRU Cache

Una caché guarda una cantidad **limitada** de datos para acceder a ellos más rápido que yendo a la fuente original (acá, un archivo en disco). La política LRU parte de la idea de que **lo que se usó hace poco es más probable que se vuelva a usar pronto**:

- Los elementos están ordenados por uso: el **primero** es el usado más recientemente y el **último** el menos reciente.
- Al consultar un elemento, pasa al principio.
- Al agregar uno nuevo, también queda al principio. Si la caché se pasa de su capacidad, se **descarta el último**.

| Primitiva | Qué hace |
|---|---|
| crear | Inicializa la caché con capacidad máxima y función de comparación de claves |
| agregar | Agrega al inicio. Si la clave ya existía, la reemplaza y queda al inicio. Si hay desborde, elimina el último |
| obtener | Busca por clave. Si existe, la mueve al inicio y devuelve su valor. Si no, informa que no existe |
| borrar | Elimina un elemento por clave |
| vaciar | Elimina todos los elementos y libera la memoria |

El proyecto tiene **tres partes**:

| Parte | Qué es | Archivos principales |
|---|---|---|
| **1** | LRU Cache con **lista simple** (cada operación recorre la lista) | `lista`, `lruCacheLista` |
| **2** | LRU Cache con **lista doble + árbol binario de búsqueda** como índice | `listaDoble`, `arbol`, `lruCacheArbol` |
| **3** | Generador de lotes de prueba y simulador de la red social Tweener | `generadorLotes`, `simuladorTweener`, `registros.h` |

---

## 2. Mapa del proyecto

```
lruCache/
├── lruCache.cbp            Proyecto Code::Blocks (targets Debug y Release)
├── LICENSE                 GNU GPL v3
├── .gitignore              Ignora objetos, ejecutables y archivos de depuración
│
├── utilidades.h/.c         Base común (códigos de retorno, tCmp, formato [clave][dato])
│
├── lista.h/.c              TDA lista simple                          ─┐ Parte 1
├── lruCacheLista.h/.c      LRU Cache con lista simple                ─┘
│
├── listaDoble.h/.c         TDA lista doble con extremos ini y fin    ─┐
├── arbol.h/.c              TDA árbol binario de búsqueda             ─┤ Parte 2
├── lruCacheArbol.h/.c      LRU Cache con lista doble + árbol índice  ─┘
│
├── registros.h             Estructuras de los archivos binarios      ─┐
├── generadorLotes.h/.c     Generador de lotes con semilla            ─┤ Parte 3
├── simuladorTweener.h/.c   Simulación y tabla de resultados          ─┤
├── testCruzado.h/.c        Test de consistencia Parte 1 vs Parte 2   ─┤
└── main.c                  Menú principal y lectura de datos         ─┘
```

### Dependencias entre archivos

```
                       utilidades
                      /     |      \
                lista   listaDoble  arbol
                  |           \     /
          lruCacheLista     lruCacheArbol
                  \           /
        simuladorTweener   testCruzado        generadorLotes
                  \           |                  /
                   \          |                 /
                           main.c  ◄── registros.h (usado por generador y simulador)
```

Los TDAs básicos (`lista`, `listaDoble`, `arbol`) **no saben nada de cachés**: solo guardan bloques de bytes. La lógica LRU vive únicamente en `lruCacheLista` y `lruCacheArbol`.

### Qué hace cada archivo

| Archivo | Tipo | Responsabilidad |
|---|---|---|
| `utilidades.h` | header | Define `MINIMO`, los códigos de retorno (`TODO_OK`, `SIN_MEM`, `NO_EXISTE`, `LISTA_VACIA`, `PARAM_INVALIDO`), el tipo `tCmp` y declara `cmpInt`, `crearEntrada`, `copiarDatoEntrada`. Incluye `stdlib.h` y `string.h` para todos los demás |
| `utilidades.c` | código | `cmpInt` (comparador de enteros sin desbordamiento), `crearEntrada` (arma el bloque `[clave][dato]`), `copiarDatoEntrada` (extrae solo el dato) |
| `lista.h` / `lista.c` | TDA | Lista simple genérica (`info`, `tamInfo`, `sig`) con primitivas de enlace (`buscarClaveLista`, `moverAlComienzoLista`, `eliminarNodoLista`) |
| `lruCacheLista.h` / `.c` | caché | **Parte 1.** Define `tLruCacheLista` y las cinco primitivas del enunciado sobre la lista simple |
| `listaDoble.h` / `.c` | TDA | Lista doble genérica con **dos extremos** (`ini`, `fin`). Mover al inicio y eliminar un nodo son O(1) |
| `arbol.h` / `.c` | TDA | Árbol binario de búsqueda genérico, no balanceado. Trabaja con **enlaces** (`tArbol *`) |
| `lruCacheArbol.h` / `.c` | caché | **Parte 2.** Define `tLruCacheArbol`: combina lista doble (orden de uso y datos) y árbol (índice por clave). Incluye `alturaLruCacheArbol` |
| `registros.h` | header | Constantes y estructuras de archivo: `tPosteo`, `tUsuario`, nombres de archivos |
| `generadorLotes.h` / `.c` | aplicación | Genera `posteos.dat` y `usuarios_<tipo>.dat` a partir de una semilla |
| `simuladorTweener.h` / `.c` | aplicación | Corre la simulación sin caché, con Parte 1 y con Parte 2, y muestra la tabla |
| `testCruzado.h` / `.c` | prueba | Aplica las mismas operaciones aleatorias a ambas cachés y verifica que sean equivalentes y consistentes; detecta fugas |
| `main.c` | aplicación | Menú, lectura de números por consola con valores por defecto |

---

## 3. Compilar y ejecutar

### Con Code::Blocks

Abrir `lruCache.cbp` y elegir un target:

| Target | Opciones | Uso |
|---|---|---|
| **Debug** | `-g -DCONTAR_MEM` y `-Wl,--wrap=malloc -Wl,--wrap=free` | Activa el contador de `malloc`/`free` del test cruzado (detecta fugas y dobles liberaciones) |
| **Release** | `-O2 -s` | Para **medir tiempos** de la simulación |

> Los tiempos de la simulación se deben medir con **Release**. En Debug el contador de memoria agrega costo a cada `malloc`/`free` y distorsiona los resultados.

### Por consola (gcc)

```bash
# Release (medición de tiempos)
gcc -Wall -O2 *.c -o lruCache

# Debug con detección de fugas
gcc -Wall -g -DCONTAR_MEM *.c -o lruCache_dbg -Wl,--wrap=malloc -Wl,--wrap=free
```

### Directorio de trabajo

Los archivos `.dat` se crean y se leen **con ruta relativa**, es decir, en el directorio de trabajo del programa. Generar los lotes y correr la simulación debe hacerse **desde el mismo directorio**. En Code::Blocks se puede revisar en *Project → Properties → Build targets → Execution working dir*.

### Menú principal

```
===== LRU Cache =====
1. Generar lote concentrado
2. Generar lote disperso
3. Generar lote ordenado
4. Test cruzado (Parte 1 vs Parte 2)
5. Simulacion Tweener
0. Salir
```

Flujo típico: **1, 2 y 3** (generar los lotes) → **4** (verificar las cachés) → **5** (simular).

---

## 4. Base común: `utilidades` y formato de entrada `[clave][dato]`

### 4.1. Códigos de retorno (`utilidades.h`)

| Código | Valor | Significado |
|---|---|---|
| `TODO_OK` | 0 | Operación exitosa |
| `SIN_MEM` | 1 | Falló un `malloc` |
| `NO_EXISTE` | 2 | La clave no está en la caché |
| `LISTA_VACIA` | 3 | Se pidió sacar de una lista vacía |
| `PARAM_INVALIDO` | 4 | Parámetros incorrectos (en `crear...` o en el generador) |

### 4.2. El comparador `tCmp`

```c
typedef int (*tCmp)(const void *a, const void *b);
```

Devuelve **negativo, cero o positivo** según `a` sea menor, igual o mayor que `b`, tal como pide el enunciado. `cmpInt` lo hace con `(x > y) - (x < y)` en lugar de `x - y`, para que no haya desbordamiento con enteros grandes.

### 4.3. El formato de una entrada: un único bloque `[clave][dato]`

Cada elemento de la caché se guarda en **un solo bloque de memoria** con la clave al principio y el dato a continuación:

```
 info (un solo malloc)
┌─────────────┬────────────────────────┐
│    clave    │         dato           │
│ (tamClave)  │       (tamDato)        │
└─────────────┴────────────────────────┘
```

Esto resuelve la **genericidad** y la **comparación** a la vez:

- La caché no sabe qué tipos de clave y dato maneja: solo copia bytes (`memcpy`) con los tamaños que le indican.
- Como la clave está **al inicio** del bloque, el comparador del usuario (`cmpInt`, por ejemplo) se aplica directamente al `info` del nodo, sin adaptadores.

| Función | Qué hace |
|---|---|
| `crearEntrada(clave, tamClave, dato, tamDato)` | Hace un `malloc(tamClave + tamDato)` y copia clave y dato. Devuelve `NULL` si no hay memoria. El llamador debe hacer `free` |
| `copiarDatoEntrada(entrada, tamEntrada, tamClave, dato, tamDato)` | Copia a `dato` la parte posterior a la clave, hasta `tamDato` bytes (nunca más de lo que hay) |

---

## 5. PARTE 1: caché con lista simple

### 5.1. Archivos

| Archivo | Rol |
|---|---|
| `lista.h` / `lista.c` | TDA lista simple |
| `lruCacheLista.h` / `lruCacheLista.c` | La caché LRU de la Parte 1 |

### 5.2. Estructuras

**Nodo y lista (`lista.h`)**

```c
typedef struct sNodo
{
    void            *info;      /* bloque [clave][dato] */
    unsigned        tamInfo;    /* tamaño del bloque */
    struct sNodo    *sig;
} tNodo;

typedef tNodo *tLista;          /* la lista es el puntero al primer nodo */
```

**Caché (`lruCacheLista.h`)**

```c
typedef struct
{
    tLista lista;          /* inicio = usado más recientemente */
    unsigned cantidad;     /* elementos guardados ahora */
    unsigned capacidad;    /* máximo permitido */
    unsigned tamClave;     /* todas las claves de una caché tienen el mismo tamaño */
    tCmp cmp;              /* comparador propio de la caché */
} tLruCacheLista;
```

La lista **es** la caché: ahí viven los datos y el orden de uso.

```
lista ─► [clave|dato] ─► [clave|dato] ─► [clave|dato] ─► NULL
         más reciente                     menos reciente
```

### 5.3. Primitivas del TDA lista (`lista.c`)

| Primitiva | Descripción |
|---|---|
| `crearLista` | Deja la lista en `NULL` |
| `vaciarLista` | Libera cada `info` y cada nodo |
| `ponerAlComienzoLista` | Hace `malloc` del nodo y de `info`, copia el dato y lo enlaza al inicio. Devuelve `SIN_MEM` si falla (sin dejar nada a medias) |
| `sacarUltimoLista` | Recorre hasta el último, copia su `info` si `dato != NULL` (hasta `cantBytes`), lo libera y lo desenlaza. `LISTA_VACIA` si no hay nodos |
| `buscarClaveLista` | Devuelve el **enlace** (`tLista *`) al nodo cuya clave coincide, o el enlace `NULL` del final si no existe |
| `moverAlComienzoLista` | Reenlaza el nodo apuntado al inicio. **No** hace `malloc` ni `free` |
| `eliminarNodoLista` | Desenlaza y libera el nodo apuntado por un enlace |

> **Por qué se devuelven enlaces (`tLista *`) y no nodos.** Con el enlace se puede desenlazar el nodo sin necesitar un puntero al anterior. Además, la misma búsqueda sirve después para mover o eliminar, sin buscar dos veces.

### 5.4. Primitivas de la caché (`lruCacheLista.c`)

```c
int  crearLruCacheLista (tLruCacheLista *c, unsigned capacidad, unsigned tamClave, tCmp cmp);
int  agregarLruCacheLista(tLruCacheLista *c, const void *clave, const void *dato, unsigned tamDato);
int  obtenerLruCacheLista(tLruCacheLista *c, const void *clave, void *dato, unsigned tamDato);
int  borrarLruCacheLista (tLruCacheLista *c, const void *clave);
void vaciarLruCacheLista (tLruCacheLista *c);
```

| Primitiva | Cómo funciona | Retornos |
|---|---|---|
| `crear` | Valida parámetros e inicializa la lista y los campos | `TODO_OK`, `PARAM_INVALIDO` (si `c` o `cmp` son `NULL`, o capacidad o tamaño de clave son 0) |
| `obtener` | `buscarClaveLista` recorre la lista. Si encuentra: `moverAlComienzoLista` + `copiarDatoEntrada`. Si llega al final: no existe | `TODO_OK`, `NO_EXISTE` |
| `agregar` | Busca la clave; si ya estaba, `eliminarNodoLista` (y `cantidad--`). Arma la entrada con `crearEntrada`, la pone al inicio y libera el bloque temporal. Si `cantidad > capacidad`, `sacarUltimoLista` | `TODO_OK`, `SIN_MEM` |
| `borrar` | Busca y elimina el nodo | `TODO_OK`, `NO_EXISTE` |
| `vaciar` | `vaciarLista` y `cantidad = 0` | — |

**Flujo de `agregar` (clave nueva, caché llena):**

```
antes:   lista ─► [A] ─► [B] ─► [C]           capacidad 3, cantidad 3
agregar D
paso 1:  buscar D (recorre toda la lista) → no está
paso 2:  poner [D] al inicio    ─► [D] ─► [A] ─► [B] ─► [C]    cantidad 4 > 3
paso 3:  sacarUltimo (recorre toda la lista) ─► [D] ─► [A] ─► [B]
```

### 5.5. Costo y resultado esperado

| Operación | Costo |
|---|---|
| obtener | O(n): recorre hasta encontrar o hasta el final |
| agregar | O(n): busca duplicado + recorre para sacar el último |
| borrar | O(n) |

Con n hasta la capacidad. **Un fallo (miss) cuesta unos tres recorridos**: el `obtener` que no encontró, el `agregar` que busca duplicado y el `sacarUltimo` al desbordar.

**Qué se espera en la simulación:** cuando hay muchos aciertos, la caché ahorra lecturas de archivo y gana. Cuando casi no hay aciertos (lote disperso u ordenado), el recorrido de la lista es costo puro y **el tiempo crece con la capacidad**: es el problema que resuelve la Parte 2.

---

## 6. PARTE 2: caché con lista doble + árbol índice

### 6.1. Archivos

| Archivo | Rol |
|---|---|
| `listaDoble.h` / `.c` | Lista doble con extremos `ini` y `fin` |
| `arbol.h` / `.c` | Árbol binario de búsqueda (índice) |
| `lruCacheArbol.h` / `.c` | La caché de la Parte 2 |

### 6.2. Estructuras

**Lista doble (`listaDoble.h`)**

```c
typedef struct sNodoD
{
    void            *info;      /* bloque [clave][dato] */
    unsigned        tamInfo;
    struct sNodoD   *sig, *ant;
} tNodoD;

typedef struct
{
    tNodoD *ini;                /* usado más recientemente */
    tNodoD *fin;                /* menos usado */
} tListaD;
```

A diferencia de la lista de la cátedra (que guarda un único puntero a un nodo cualquiera), acá se guardan **los dos extremos**. Así, mover al inicio y descartar el último **nunca recorren** la lista.

**Árbol (`arbol.h`)**

```c
typedef struct sNodoA
{
    void            *dato;      /* bloque [clave][puntero al nodo de la lista] */
    unsigned        tam;
    struct sNodoA   *izq, *der;
} tNodoA;

typedef tNodoA *tArbol;
```

**Caché (`lruCacheArbol.h`)**

```c
typedef struct
{
    tListaD lista;         /* ini = más reciente, fin = menos */
    tArbol indice;
    unsigned cantidad;
    unsigned capacidad;
    unsigned tamClave;
    tCmp cmp;
} tLruCacheArbol;
```

### 6.3. Cómo se combinan las dos estructuras

| Estructura | Rol | Qué guarda cada nodo |
|---|---|---|
| **Lista doble** | **Es la caché**: dueña de los datos y del orden de uso | `[clave][dato]` |
| **Árbol** | **Índice**: encuentra rápido el nodo de una clave | `[clave][puntero al nodo de la lista]` |

```
                 ÁRBOL (índice, ordenado por clave)
                        [30|•]
                       /      \
                  [10|•]      [50|•]
                     │           └───────────────┐
        ┌────────────┘                           │
        ▼                                        ▼
 ini ─► [30|dato] ◄─► [10|dato] ◄─► [50|dato] ◄─ fin
        más reciente                  menos reciente
                 LISTA DOBLE (orden de uso)
```

- La **búsqueda por clave** se hace en el árbol.
- El **orden de uso** se mantiene en la lista.
- Cada nodo del árbol tiene su **propia copia de la clave** al inicio de su bloque (por eso el comparador funciona directo) seguida del puntero al nodo de la lista que contiene el dato real.

### 6.4. Primitivas de la lista doble (`listaDoble.c`)

| Primitiva | Descripción | Costo |
|---|---|---|
| `crearListaDoble` | `ini = fin = NULL` | O(1) |
| `vaciarListaDoble` | Libera todos los nodos y sus `info`. Devuelve cuántos liberó | O(n) |
| `ponerAlInicioListaDoble` | Crea el nodo, copia el dato y lo deja en `ini`. Si estaba vacía, también actualiza `fin` | O(1) |
| `moverAlInicioListaDoble` | Desengancha el nodo de su lugar (actualizando `fin` si era el último) y lo reengancha en `ini`. Sin `malloc`/`free` | O(1) |
| `eliminarNodoListaDoble` | Reconecta vecinos (o `ini`/`fin` si era extremo), libera `info` y nodo | O(1) |

### 6.5. Primitivas del árbol (`arbol.c`)

| Primitiva | Descripción |
|---|---|
| `crearArbol` | Deja el árbol en `NULL` |
| `vaciarArbol` | Libera nodos y datos **sin recursión**: rota a la derecha hasta que la raíz no tiene hijo izquierdo y entonces la libera. No desborda la pila ni con un árbol degenerado |
| `alturaArbol` | Cantidad de niveles (recursiva) |
| `buscarEnlaceArbol` | Baja comparando con `cmp`. Devuelve el enlace al nodo con esa clave **o el enlace `NULL` donde se insertaría** |
| `insertarEnEnlaceArbol` | Crea un nodo (copia del dato) en un enlace ya encontrado. No vuelve a buscar |
| `eliminarEnlaceArbol` | Elimina el nodo de un enlace. Con dos hijos intercambia los datos con el sucesor inorden y elimina ese nodo (que tiene a lo sumo un hijo) |

### 6.6. Funciones internas de `lruCacheArbol.c`

Toda alta y toda baja pasan por estas funciones, que actualizan **siempre las dos estructuras**:

| Función (`static`) | Qué hace |
|---|---|
| `nodoDeIndice` | Lee del nodo del índice el puntero al nodo de la lista (con `memcpy`, porque la posición dentro del bloque puede quedar desalineada) |
| `quitarEntrada` | Lee el puntero a la lista, elimina del árbol, elimina de la lista, `cantidad--`. El orden importa: el puntero se lee **antes** de tocar el árbol |
| `eliminarUltimo` | Toma la clave del nodo `lista.fin` (todavía existe), la busca en el árbol y llama a `quitarEntrada` |
| `insertarEntrada` | Crea la entrada en la lista y luego en el árbol (en el enlace ya buscado). Si falla el árbol por falta de memoria, **deshace** el alta en la lista. Si hay desborde, `eliminarUltimo` |

### 6.7. Primitivas de la caché

```c
int  crearLruCacheArbol (tLruCacheArbol *c, unsigned capacidad, unsigned tamClave, tCmp cmp);
int  agregarLruCacheArbol(tLruCacheArbol *c, const void *clave, const void *dato, unsigned tamDato);
int  obtenerLruCacheArbol(tLruCacheArbol *c, const void *clave, void *dato, unsigned tamDato);
int  borrarLruCacheArbol (tLruCacheArbol *c, const void *clave);
void vaciarLruCacheArbol (tLruCacheArbol *c);
unsigned alturaLruCacheArbol(const tLruCacheArbol *c);   /* altura del árbol índice */
```

| Primitiva | Cómo funciona |
|---|---|
| `crear` | Mismas validaciones que la Parte 1; inicializa lista, árbol y campos |
| `obtener` | Busca en el árbol → lee el puntero al nodo → `moverAlInicioListaDoble` (O(1)) → copia el dato. `NO_EXISTE` si no está |
| `agregar` | Busca en el árbol. Si la clave existía, la quita de **ambas** estructuras y vuelve a buscar (el árbol cambió). Inserta en lista y árbol; si hay desborde descarta el último |
| `borrar` | Busca en el árbol y llama a `quitarEntrada` |
| `vaciar` | `vaciarArbol` + `vaciarListaDoble` (cada una libera solo lo suyo) |
| `alturaLruCacheArbol` | Devuelve `alturaArbol` del índice (primitiva extra pedida por el enunciado) |

**Flujo de `obtener` (acierto):**

```
obtener 10
1. buscarEnlaceArbol(10)         → encuentra [10|•]            O(h)
2. nodoDeIndice                  → nodo de lista de la clave 10
3. moverAlInicioListaDoble       → ini ─► [10|dato] ◄─► [30|dato] ◄─► [50|dato] ◄─ fin   O(1)
4. copiarDatoEntrada             → copia el dato al llamador
```

**Flujo de `agregar` con desborde (clave nueva, caché llena):**

```
1. buscarEnlaceArbol(clave)      → enlace NULL (no existe)
2. ponerAlInicioListaDoble       → nodo nuevo en ini
3. insertarEnEnlaceArbol         → nodo nuevo en el árbol, en el enlace del paso 1
4. cantidad > capacidad          → eliminarUltimo:
     clave = fin->info
     buscar en el árbol → eliminarEnlaceArbol → eliminarNodoListaDoble(fin)
```

### 6.8. Costo y resultado esperado

| Operación | Costo |
|---|---|
| Buscar por clave | O(h), con h la altura del árbol |
| Mover al inicio / eliminar un nodo de la lista | O(1) |
| Descartar el último | O(h) en el árbol + O(1) en la lista |

- Con claves en **orden aleatorio**, h ≈ log n: la Parte 2 queda casi **plana** al crecer la capacidad y le gana a la Parte 1 en cuanto la capacidad es grande.
- Con claves en **orden creciente** (lote ordenado), el árbol no se balancea y se degenera en una cadena: **h = n**. La búsqueda vuelve a ser O(n) y, además de recorrer el árbol, se mantiene la lista doble y se hacen más reservas de memoria, así que la Parte 2 resulta **más lenta que la Parte 1**. La altura informada coincide con la capacidad.

---

## 7. Manejo de memoria (Partes 1 y 2)

Como el dato está referenciado desde dos estructuras en la Parte 2, la regla de diseño es de **propiedad única**:

| Memoria | Dueño | Quién la libera |
|---|---|---|
| Bloque `[clave][dato]` y nodo de la lista (simple o doble) | Lista | `vaciarLista...` / `eliminarNodoLista...` |
| Bloque `[clave][puntero]` y nodo del árbol | Árbol | `vaciarArbol` / `eliminarEnlaceArbol` |

Reglas concretas:

- El árbol **nunca** libera el nodo de la lista: solo lo referencia. La lista **nunca** toca el árbol.
- El árbol tiene su **propia copia de la clave**, así no depende de memoria de la lista.
- `quitarEntrada` lee el puntero a la lista **antes** de modificar el árbol.
- `eliminarUltimo` lee la clave del nodo de la lista **antes** de liberarlo.
- Al eliminar un nodo del árbol con dos hijos se intercambian los **datos** (no los nodos). Cada bloque `[clave][puntero]` viaja completo, así que clave y puntero nunca quedan desacoplados, y los nodos de la lista siguen apuntados correctamente.
- Los bloques temporales de `crearEntrada` se liberan en la misma función que los crea (también en los caminos de error).
- Si una inserción falla por falta de memoria, no queda nada a medias: `ponerAlComienzo...` libera el nodo si falla el `malloc` de `info`, y `insertarEntrada` deshace el alta en la lista si falla el árbol.

**Cómo se verifica:** con el target **Debug**, el test cruzado cuenta cada `malloc` y `free` de los TDAs (`--wrap`) y reporta el balance, que debe ser **0** en todos los escenarios (si fuera negativo, habría doble liberación; positivo, fuga).

---

## 8. PARTE 3: simulación Tweener

**Tweener** es una red social que solo admite posteos de menos de 140 caracteres. Se quiere validar si una LRU Cache mejora el tiempo de mostrar el feed de cada usuario.

### 8.1. `registros.h`: estructuras de los archivos

| Constante | Valor | Significado |
|---|---|---|
| `CANT_POSTEOS` | 50000 | Cantidad de posteos |
| `LARGO_TEXTO` | 141 | `char texto[141]` (hasta 139 caracteres + terminador) |
| `LARGO_USUARIO` | 15 | `char usuario[15]` |
| `LARGO_FEED` | 20 | Posteos por feed |
| `NOMBRE_POSTEOS` | `"posteos.dat"` | Archivo de posteos |
| `FORMATO_USUARIOS` | `"usuarios_%s.dat"` | `%s` = `concentrado`, `disperso` u `ordenado` |

```c
typedef struct { int id; char texto[LARGO_TEXTO]; } tPosteo;      /* id = posición del registro */
typedef struct { char usuario[LARGO_USUARIO]; int feed[LARGO_FEED]; } tUsuario;
```

Como el `id` coincide con la **posición del registro**, un posteo se lee directamente con `fseek(id * sizeof(tPosteo))` + `fread`, sin buscar.

### 8.2. `generadorLotes.c`: generador de lotes

Genera dos tipos de archivo binario:

| Archivo | Contenido |
|---|---|
| `posteos.dat` | 50000 registros `tPosteo`. Texto aleatorio de 1 a 139 caracteres (letras minúsculas y espacio); el resto del registro queda en 0 (archivo reproducible byte a byte) |
| `usuarios_<tipo>.dat` | `usuarios` registros `tUsuario`, con nombres `usuario00001`, `usuario00002`, … y 20 ids de posteo en el feed según el tipo de lote |

**Tipos de lote** (cada uno es una función generadora de ids, elegida por nombre en una tabla):

| Lote | Función | Cómo se generan los ids | Situación que representa |
|---|---|---|---|
| **concentrado** | `idConcentrado` | Siempre se elige dentro de un *pool* de posteos populares (`tamPool` ids sacados al azar al inicio) | Muchos usuarios ven los mismos posteos |
| **disperso** | `idDisperso` | Cualquier id de 0 a 49999 con igual probabilidad | Los usuarios ven posteos mayormente distintos |
| **ordenado** | `idOrdenado` | `id = contador × 50000 / total`, estrictamente creciente en **toda** la simulación | Los ids aparecen en orden creciente |

Detalles internos:

| Función (`static`) | Qué hace |
|---|---|
| `aleatorio(n)` | Número en `[0, n)` combinando dos `rand()` (en sentencias separadas, para que el orden esté definido y se cubran rangos mayores que `RAND_MAX`) |
| `buscarTipo` | Busca el generador por nombre en la tabla `tTipoLote`; `NULL` si no existe |
| `generarTexto` | Llena el texto con 1 a 139 caracteres |
| `generarPosteos` | Escribe `posteos.dat` |
| `generarUsuarios` | Escribe el archivo de usuarios llamando al generador del lote 20 veces por usuario |
| `generarLote` | Valida, siembra (`srand`), arma el pool, genera ambos archivos y avisa |

**Orden de uso del generador aleatorio** (importante para la reproducibilidad): `srand(semilla)` → pool → `posteos.dat` → usuarios. La **misma semilla, tipo y parámetros** reproducen exactamente el mismo lote.

**Validaciones de `generarLote`** (se hacen **antes** de abrir ningún archivo, por lo que ante un error no se pisa nada):

| Condición | Resultado |
|---|---|
| Tipo inexistente, `usuarios < 200` o `> 99999`, `pool < 1` o `> 50000` | Imprime `Parametros invalidos (usuarios 200 a 99999, pool 1 a 50000)` y devuelve `PARAM_INVALIDO` |
| Lote ordenado con `usuarios × 20 > 50000` (más de 2500 usuarios) | Imprime el máximo permitido y devuelve `PARAM_INVALIDO` |
| Sin memoria para el pool | `Sin memoria`, devuelve `SIN_MEM` |
| Error de escritura | `Error al escribir los archivos` |
| Todo bien | `Lote 'tipo' (semilla N): 50000 posteos en posteos.dat, U usuarios en usuarios_tipo.dat` |

### 8.3. `simuladorTweener.c`: la simulación

Para **cada usuario** se busca cada uno de los 20 posteos de su feed. Los datos leídos **no se muestran, no se imprimen ni se guardan**: solo se mide el tiempo.

```
para cada posteo del feed:
    obtener de la caché
    ├─ si está  ──► ACIERTO (no se toca el archivo)
    └─ si no    ──► FALLO: leer posteos.dat (fseek + fread) y agregarlo a la caché
```

**Tres variantes** sobre el mismo lote, todas con la misma interfaz mediante una tabla de funciones (`tEstrategia`), de modo que no hay código duplicado por variante:

| Variante | Qué es |
|---|---|
| `sin cache` | Siempre lee del archivo (línea base). Sus adaptadores nunca encuentran ni guardan nada |
| `parte 1` | Usa `tLruCacheLista` |
| `parte 2` | Usa `tLruCacheArbol` (informa además la altura del árbol) |

Cada variante con caché se ejecuta con capacidades **10, 100, 1000 y 10000** (arreglo `capacidades` dentro de `ejecutarSimulacion`; para probar otras, se edita ahí). Una unión (`tCache`) permite que las dos cachés compartan el mismo espacio.

| Función (`static` salvo `ejecutarSimulacion`) | Qué hace |
|---|---|
| `cargarUsuarios` | Lee **todo** el archivo de usuarios a memoria (para que su lectura no entre en la medición). Cantidad = tamaño del archivo / `sizeof(tUsuario)` |
| `leerPosteo` | `fseek` + `fread` directo y comprueba que el `id` leído coincida con el pedido |
| `procesarUsuario` | Procesa los 20 posteos de un feed y actualiza aciertos, fallos y lecturas |
| `realizarSimulacion` | Una simulación completa con una caché **nueva** (crea, procesa todos los usuarios, toma la altura y vacía) |
| `medirYMostrarFila` | Repite la simulación N veces, promedia el tiempo con `clock()` e imprime una fila |
| `ejecutarSimulacion` | Carga archivos, imprime el encabezado y recorre variantes y capacidades |

### 8.4. Cómo se miden los tiempos

El tiempo de leer **un** posteo es menor que la resolución del reloj, por lo que no se mide individualmente: se mide el **total** de la simulación y los tiempos por posteo y por usuario se calculan como **promedio**, como pide el enunciado. Para que el valor sea estable se repite la simulación completa `Repeticiones` veces (cada vez con caché nueva) y se divide por N.

Los contadores **no** se promedian: son idénticos en todas las repeticiones (el lote es el mismo y LRU es determinista).

> `clock()` mide tiempo de CPU del proceso. El tiempo incluye crear y vaciar la caché de cada repetición. El sistema operativo mantiene `posteos.dat` en su propia caché, por lo que leer del archivo suele ser muy barato (del orden de microsegundos) y cada acierto ahorra poco. Lo importante son las **tendencias** y los **contadores**, no los valores absolutos.

### 8.5. Columnas de la tabla

| Columna | Significado |
|---|---|
| `variante` | `sin cache`, `parte 1` o `parte 2` |
| `capacid.` | Capacidad de la caché (`-` si no aplica) |
| `total(ms)` | Tiempo de **una** simulación completa (promedio de las repeticiones) |
| `us/usuario` | Tiempo total dividido por la cantidad de usuarios |
| `us/posteo` | Tiempo total dividido por (usuarios × 20) |
| `aciertos` | Posteos encontrados en la caché |
| `fallos` | Posteos no encontrados en la caché |
| `lecturas` | Lecturas realizadas a `posteos.dat` |
| `altura` | Altura del árbol índice al final de la simulación (solo Parte 2) |

**Relaciones que siempre se cumplen** (sirven para validar una corrida):

- `aciertos + fallos = usuarios × 20`
- `lecturas = fallos` (cada fallo hace exactamente una lectura)
- `sin cache`: todo son fallos
- Los **aciertos y fallos son idénticos** en Parte 1 y Parte 2 para la misma capacidad, porque aplican la misma política LRU

---

## 9. Ingreso de datos: qué pasa con cada entrada

Toda la lectura por consola pasa por una única función, `leerNumero(mensaje, defecto)` en `main.c`. Muestra el mensaje con el valor por defecto entre corchetes, por ejemplo:

```
Semilla [42]:
```

### 9.1. Reglas de `leerNumero`

La línea se lee con `fgets` (buffer de 32 bytes) y se interpreta con `strtoul` en base 10:

| Lo que se escribe | Qué ocurre | Ejemplo con `Semilla [42]` |
|---|---|---|
| Solo **Enter** | Usa el valor por defecto | → semilla 42 |
| Un número | Usa ese número | `7` → semilla 7 |
| Espacios y después un número | Los espacios iniciales se ignoran | `  7` → 7 |
| Número seguido de texto | Toma el número y **ignora el resto** | `7abc` → 7 · `3.9` → 3 |
| Texto que no empieza con un número | No hay dígitos: usa el valor por defecto | `abc` → 42 |
| Fin de entrada (Ctrl+D / Ctrl+Z, o entrada redirigida agotada) | Usa el valor por defecto | → 42 |
| `0` | Es un número válido (**no** equivale a Enter) | semilla 0 |
| Prefijo hexadecimal `0x10` | Se interpreta en base 10: lee el `0` y ignora `x10` | → 0 |
| Número negativo `-5` | `strtoul` lo acepta y lo envuelve a un valor enorme; al convertirlo a `int` queda negativo (en la práctica, `-5`), y será rechazado donde corresponda | ver 9.3 |
| Número muy grande (desborda) | `strtoul` devuelve el máximo; al convertir a `int` queda `-1` | ver 9.3 |
| Más de 31 caracteres | `fgets` toma 31 y **el resto queda en el teclado** y será leído como respuesta de la **pregunta siguiente** | evitar líneas largas |

### 9.2. Menú principal (`Opcion [0]`)

El valor por defecto del menú es **0 = Salir**:

| Entrada | Resultado |
|---|---|
| `1`, `2`, `3` | Genera lote concentrado, disperso u ordenado |
| `4` | Test cruzado |
| `5` | Simulación |
| `0` | Sale |
| **Enter** (sin escribir nada) | **Sale del programa** (el defecto es 0) |
| Texto no numérico | Equivale a Enter: **sale** |
| Fin de entrada | **Sale** (así el programa no entra en bucle infinito con entrada redirigida) |
| Cualquier otro número (`7`, `-1`) | Imprime `Opcion invalida.` y vuelve a mostrar el menú |

### 9.3. Generar un lote (opciones 1, 2 y 3)

Preguntas, en orden:

| Pregunta | Defecto | Rango válido | Se pregunta en |
|---|---|---|---|
| `Semilla` | 42 | cualquier número (se convierte a `unsigned` para `srand`) | los 3 lotes |
| `Usuarios` | 200 (`USUARIOS_MIN`) | 200 a 99999 | los 3 lotes |
| `Posteos populares (pool)` | 100 (`POOL_DEFECTO`) | 1 a 50000 | **solo** lote concentrado |

Casos concretos:

| Caso | Qué pasa |
|---|---|
| Enter en las tres preguntas (concentrado) | Semilla 42, 200 usuarios, pool de 100 → lote reproducible estándar |
| Misma semilla, tipo y parámetros dos veces | Archivos **idénticos** (reproducible) |
| Otra semilla (`7`) | Otro lote distinto, igualmente reproducible con `7` |
| Semilla `0` | Válida. Según la implementación de `rand`, `srand(0)` puede ser equivalente a `srand(1)` |
| Semilla enorme | Se trunca a `unsigned` (32 bits): semillas que difieren en un múltiplo de 2³² dan el mismo lote |
| Usuarios `100` (menor que 200) | `Parametros invalidos (usuarios 200 a 99999, pool 1 a 50000)`. **No se escribe ningún archivo**; los anteriores quedan intactos |
| Usuarios `-5` o desbordado | Queda negativo → mismo mensaje de parámetros inválidos |
| Usuarios `abc` | Usa el defecto, 200 |
| Usuarios `3000` en lote **ordenado** | `Lote ordenado: usuarios * 20 no puede superar 50000 (maximo 2500 usuarios)`. No se escribe nada |
| Usuarios `2500` en lote ordenado | Válido: usa exactamente los 50000 posteos, uno por cada acceso |
| Pool `1` (concentrado) | Todos los usuarios ven el **mismo** posteo. Con caché hay 1 solo fallo |
| Pool `50000` (concentrado) | Se comporta casi como un lote disperso |
| Pool `0` o `60000` | Parámetros inválidos |

Efectos que conviene saber:

- Cada generación **sobrescribe sin preguntar** `posteos.dat` y el `usuarios_<tipo>.dat` correspondiente. Los ids de los otros lotes siguen siendo válidos, porque el id es la posición del registro, pero el **texto** de los posteos cambia con la semilla (a la simulación no le afecta: no usa el texto).
- Si se genera cada lote con una semilla distinta, el último generado define el contenido de `posteos.dat`.
- `rand()` no es igual en todas las plataformas: la misma semilla puede dar lotes distintos en Windows y en Linux. Dentro de una misma máquina y compilador, es reproducible.

### 9.4. Simulación (opción 5)

| Pregunta | Defecto | Efecto |
|---|---|---|
| `Lote (1 concentrado, 2 disperso, 3 ordenado, 4 todos)` | 4 | `1` a `3`: un solo lote. `4` (o Enter): los tres lotes seguidos. Otro número (incluido `0`): imprime `Lote invalido.` y vuelve al menú |
| `Repeticiones` | 10 | Cantidad de veces que se repite cada simulación para promediar. Un valor menor que 1 (`0`, negativo) se corrige a **1** |

Casos concretos:

| Caso | Qué pasa |
|---|---|
| Enter, Enter | Los tres lotes con 10 repeticiones cada uno |
| Lote `2`, repeticiones `3` | Solo el disperso, 3 repeticiones (más rápido; útil para pruebas) |
| Repeticiones muy grandes | Funciona, pero tarda: cada lote ejecuta 9 filas (1 sin caché + 4 capacidades × 2 partes), cada una `N` veces. El lote disperso con la Parte 1 y capacidad 10000 es el más lento |
| Falta `usuarios_<tipo>.dat` | `No se pudo leer usuarios_<tipo>.dat (generalo con las opciones 1 a 3)`. Con "todos", sigue con el siguiente lote |
| Falta `posteos.dat` | `No se pudo abrir posteos.dat` |
| Se regeneró el lote con otros parámetros | La simulación usa el **último archivo generado** y calcula la cantidad de usuarios desde su tamaño |

### 9.5. Test cruzado (opción 4)

No pide datos. Corre los escenarios fijos descritos en la sección siguiente.

---

## 10. Test cruzado

La opción 4 compara la Parte 1 contra la Parte 2 con **20000 operaciones aleatorias** (50 % agregar, 40 % obtener, 10 % borrar) aplicadas por igual a ambas, con semilla fija 1234. Se prueban capacidades **1, 2, 5, 50 y 200**, y para cada una un rango de claves **igual a la capacidad** (mucho acierto) y **el triple** (mucho desborde): 10 escenarios.

Después de **cada operación** verifica:

- Ambas devuelven el mismo código y el mismo dato.
- Ambas tienen la misma cantidad de elementos y no superan la capacidad.
- Las dos listas tienen **exactamente las mismas entradas en el mismo orden**, y los enlaces `ant` y `fin` de la lista doble son correctos.
- El árbol es un BST válido y tiene tantos nodos como elementos.
- Cada nodo de la lista está en el árbol y es apuntado por su entrada del índice.

Al final, tras `vaciar`, comprueba que no quedaron restos (cantidades en 0 y punteros en `NULL`) y que el balance de `malloc`/`free` es **0**.

**Salida esperada:**

- Con **Debug** (`-DCONTAR_MEM`):

```
capacidad  rango    resultado  fugas
1          1        OK         0
1          3        OK         0
2          2        OK         0
...
200        600      OK         0

TODO OK
```

- Con **Release**: la columna de fugas muestra `(no medido)`; igualmente se verifica la consistencia y termina con `TODO OK`.
- Si algo falla: `FALLO en operacion N` (el número de la operación en que se rompió) o fuga distinta de 0, y termina con `HAY ERRORES`.

---

## 11. Resultados esperados y ejemplo

### 11.1. Qué se espera de cada lote

| Lote | Aciertos | Parte 1 | Parte 2 |
|---|---|---|---|
| **Concentrado** | Con capacidad ≥ pool, solo hay un fallo por cada posteo distinto del pool (≤ pool). Con capacidad menor que el pool, el porcentaje de aciertos ronda capacidad / pool | Gana a no usar caché si la capacidad cubre el pool | Gana y es más rápida que la Parte 1 |
| **Disperso** | Casi nulos (4000 accesos sobre 50000 posteos): ningún beneficio real | Empeora al crecer la capacidad (recorrido de la lista) | Casi plana; en el mejor caso empata con no usar caché |
| **Ordenado** | **Cero** aciertos (ningún id se repite) | Pierde contra no usar caché | Peor que la Parte 1: árbol degenerado, `altura = capacidad` |

### 11.2. Corrida de ejemplo

Release, 200 usuarios, semilla 42, 3 repeticiones. Los tiempos dependen de la máquina; importan las tendencias y los contadores.

**Lote concentrado (pool de 100)**

```
variante   capacid.  total(ms)   us/usuario   us/posteo  aciertos  fallos  lecturas altura
sin cache         -       1.64         8.19       0.409         0    4000      4000      -
parte 1          10       1.85         9.26       0.463       389    3611      3611      -
parte 2          10       2.18        10.88       0.544       389    3611      3611      4
parte 1         100       0.53         2.65       0.132      3900     100       100      -
parte 2         100       0.32         1.62       0.081      3900     100       100     13
parte 1        1000       0.53         2.67       0.134      3900     100       100      -
parte 2        1000       0.32         1.58       0.079      3900     100       100     13
parte 1       10000       0.53         2.63       0.131      3900     100       100      -
parte 2       10000       0.32         1.61       0.081      3900     100       100     13
```

Con capacidad ≥ 100 casi todo son aciertos (solo 100 fallos, uno por posteo popular). El tiempo baja unas 3 veces con la Parte 1 y unas 5 con la Parte 2. Con capacidad 10 hay pocos aciertos y la caché todavía no compensa su costo.

**Lote disperso**

```
variante   capacid.  total(ms)   us/usuario   us/posteo  aciertos  fallos  lecturas altura
sin cache         -       2.58        12.89       0.645         0    4000      4000      -
parte 1          10       2.62        13.11       0.655         1    3999      3999      -
parte 2          10       3.03        15.14       0.757         1    3999      3999      5
parte 1         100       4.73        23.66       1.183        12    3988      3988      -
parte 2         100       3.48        17.42       0.871        12    3988      3988     13
parte 1        1000      37.51       187.53       9.377        69    3931      3931      -
parte 2        1000       4.44        22.20       1.110        69    3931      3931     19
parte 1       10000      66.52       332.59      16.630       167    3833      3833      -
parte 2       10000       4.87        24.36       1.218       167    3833      3833     25
```

Ninguna variante con caché le gana a no usarla. La Parte 1 empeora al crecer la capacidad; la Parte 2 queda casi plana y su altura crece despacio (≈ log n).

**Lote ordenado**

```
variante   capacid.  total(ms)   us/usuario   us/posteo  aciertos  fallos  lecturas altura
sin cache         -       1.35         6.76       0.338         0    4000      4000      -
parte 1          10       1.66         8.31       0.415         0    4000      4000      -
parte 2          10       2.12        10.61       0.530         0    4000      4000     10
parte 1         100       3.77        18.85       0.943         0    4000      4000      -
parte 2         100       7.39        36.97       1.848         0    4000      4000    100
parte 1        1000      40.33       201.65      10.082         0    4000      4000      -
parte 2        1000      80.47       402.35      20.117         0    4000      4000   1000
parte 1       10000      66.60       332.99      16.650         0    4000      4000      -
parte 2       10000     187.84       939.17      46.959         0    4000      4000   4000
```

Sin repeticiones no hay aciertos y el árbol queda degenerado: su altura es igual a la capacidad (hasta llenarse, como con 4000 en la última fila) y la Parte 2 resulta más lenta que la Parte 1.

### 11.3. Respuestas a las preguntas del enunciado

**1. ¿En qué situaciones la caché mejora el tiempo y en cuáles no?**
Mejora cuando hay **localidad**: los mismos posteos se piden varias veces mientras siguen en la caché (lote concentrado con capacidad suficiente para el conjunto popular). No mejora, y hasta empeora, cuando casi no hay repeticiones (disperso, ordenado): la caché agrega costo (buscar, insertar, descartar) sin ahorrar lecturas. Además, leer del archivo es muy barato porque el sistema operativo ya lo mantiene en su caché, de modo que cada acierto ahorra poco.

**2. ¿A partir de qué capacidad deja de convenir la Parte 1 y por qué?**
Depende del lote. En el disperso ya no conviene desde capacidad 100 (4,73 ms contra 2,58 ms sin caché) y desde 1000 la diferencia es grande (37,5 ms). Cada `obtener`, `agregar` y descarte recorre la lista completa: el costo por operación crece linealmente con la capacidad, mientras que los aciertos casi no aumentan. En el concentrado sigue conviniendo porque los aciertos compensan el recorrido, y la Parte 2 le gana de todos modos.

**3. ¿Qué ocurre con el árbol índice en el lote ordenado y cómo afecta al rendimiento?**
Las claves llegan en orden creciente, así que cada inserción va al extremo derecho y el árbol se convierte en una **lista enlazada disfrazada**: altura igual a la cantidad de elementos (1000 con capacidad 1000, 4000 con capacidad 10000). Buscar pasa de O(log n) a O(n) y desde capacidad 100 la Parte 2 queda más lenta que la Parte 1, porque además de recorrer el árbol mantiene la lista doble y hace más reservas de memoria. La solución sería un árbol balanceado (AVL), fuera del alcance de este TP.

---

## 12. Decisiones de diseño y limitaciones

**`crear` recibe `tamClave`.** El enunciado habla de capacidad y comparador. Se agregó el tamaño de la clave porque el comparador es propio de cada caché, por lo tanto el tipo de clave es fijo por caché. A cambio, `agregar` y `obtener` no necesitan recibirlo en cada llamada.

**Nombres de las primitivas.** Los nombres del enunciado (`crear_lrucache`, etc.) se pasaron a camelCase en español por convención del proyecto: `crearLruCacheLista`, `agregarLruCacheArbol`, y así. Los sufijos `Lista` y `Arbol` permiten tener ambas implementaciones en el mismo programa.

**La lista doble guarda `ini` y `fin`.** Es necesario para que mover al inicio y descartar el último sean O(1), como exige la Parte 2.

**Los TDAs trabajan con enlaces.** Devolver `tLista *` o `tArbol *` evita buscar dos veces y permite eliminar sin puntero al anterior.

**Limitaciones conocidas:**

- La clave debe ser un valor de **tamaño fijo** (`int`, estructura, `char[N]`). Una clave `char *` solo copiaría el puntero, no el texto.
- Si falta memoria al **reemplazar** una clave existente, la entrada vieja ya fue quitada.
- `obtener` copia como máximo `tamDato` bytes. El llamador debe conocer el tamaño de su dato.
- El árbol no se balancea (se hace evidente en el lote ordenado).
- `alturaArbol` es recursiva: con un árbol degenerado de decenas de miles de nodos podría agotar la pila. `vaciarArbol` es iterativo por esa razón.
- Los archivos `.dat` se escriben con `fwrite` de estructuras completas, por lo que **no son portables** entre compiladores o plataformas con distinto padding/endianness.
- Las pruebas usan claves `int`; la genericidad con otros tipos de clave no está cubierta por el test.
