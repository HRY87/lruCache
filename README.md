# Trabajo Práctico: LRU Cache

Implementación en C de un TDA **LRU Cache** (*Least Recently Used*) en dos versiones, más una simulación de la red social ficticia **Tweener** para medir cuánto mejora (o no) el procesamiento al usar la caché.

- Lenguaje: C
- IDE: Code::Blocks 20.03 a 25.03 (proyecto `lruCache.cbp`)

---

## Índice

1. [Qué es una LRU Cache](#1-qué-es-una-lru-cache)
2. [Estructura del proyecto](#2-estructura-del-proyecto)
3. [Compilar y ejecutar](#3-compilar-y-ejecutar)
4. [Diseño de los TDAs](#4-diseño-de-los-tdas)
5. [Parte 1: caché con lista simple](#5-parte-1-caché-con-lista-simple)
6. [Parte 2: caché con lista doble y árbol índice](#6-parte-2-caché-con-lista-doble-y-árbol-índice)
7. [Manejo de memoria](#7-manejo-de-memoria)
8. [Parte 3: simulación Tweener](#8-parte-3-simulación-tweener)
9. [Test cruzado](#9-test-cruzado)
10. [Resultados de ejemplo y análisis](#10-resultados-de-ejemplo-y-análisis)
11. [Decisiones de diseño y limitaciones](#11-decisiones-de-diseño-y-limitaciones)

---

## 1. Qué es una LRU Cache

Una caché guarda una cantidad **limitada** de datos para acceder a ellos más rápido que yendo a la fuente original (acá, un archivo). La política LRU parte de la idea de que **lo que se usó hace poco es más probable que se vuelva a usar pronto**:

- Los elementos se ordenan por uso: el **primero** es el usado más recientemente y el **último** es el menos reciente.
- Al consultar un elemento, pasa al principio.
- Al agregar uno nuevo, también queda al principio. Si la caché se pasa de su capacidad, se **descarta el último**.

Primitivas del TDA (según el enunciado):

| Primitiva | Qué hace |
|---|---|
| crear | Inicializa la caché con capacidad máxima y función de comparación de claves |
| agregar | Agrega al inicio. Si la clave ya existía, la reemplaza y queda al inicio. Si hay desborde, elimina el último |
| obtener | Busca por clave. Si existe, la mueve al inicio y devuelve su valor. Si no, informa que no existe |
| borrar | Elimina un elemento por clave |
| vaciar | Elimina todos los elementos y libera la memoria |

---

## 2. Estructura del proyecto

```
lru_cache/
├── lruCache.cbp           Proyecto Code::Blocks (Debug y Release)
│
├── utilidades.h/.c        Códigos de retorno, tCmp, cmpInt, armado de entradas [clave][dato]
│
├── lista.h/.c             TDA lista simple
├── listaDoble.h/.c        TDA lista doble (con extremos ini y fin)
├── arbol.h/.c             TDA árbol binario de búsqueda
│
├── lruCacheLista.h/.c     LRU Cache Parte 1 (lista simple)
├── lruCacheArbol.h/.c     LRU Cache Parte 2 (lista doble + árbol índice)
│
├── registros.h            Estructuras de archivo: tPosteo y tUsuario
├── generadorLotes.h/.c    Generador de lotes de prueba (con semilla) y exportación a .txt
├── simuladorTweener.h/.c  Simulación Tweener y tabla de resultados
├── testCruzado.h/.c       Comparación Parte 1 vs Parte 2 y detección de fugas
└── main.c                 Menú principal
```

Separación de responsabilidades en tres capas:

| Capa | Archivos | Responsabilidad |
|---|---|---|
| TDAs básicos | `lista`, `listaDoble`, `arbol` | Estructuras de datos genéricas, sin saber nada de cachés |
| TDAs de caché | `lruCacheLista`, `lruCacheArbol` | Solo la lógica LRU, apoyada en los TDAs básicos |
| Aplicación | `generadorLotes`, `simuladorTweener`, `testCruzado`, `main` | Parte 3 y pruebas |

Archivos que genera el programa al ejecutarse (no forman parte del código fuente):

| Archivo | Contenido |
|---|---|
| `posteos.dat` / `posteos.txt` | Los 50000 posteos, en binario y en texto legible |
| `usuarios_<tipo>.dat` / `usuarios_<tipo>.txt` | Los usuarios de cada lote, en binario y en texto legible |

---

## 3. Compilar y ejecutar

### Con Code::Blocks

Abrir `lruCache.cbp` y compilar con uno de los dos targets:

| Target | Opciones | Uso |
|---|---|---|
| **Debug** | `-g -DCONTAR_MEM` y `-Wl,--wrap=malloc -Wl,--wrap=free` | Activa el contador de `malloc`/`free` del test cruzado para detectar fugas |
| **Release** | `-O2 -s` | Para **medir tiempos** de la simulación |

> Los tiempos de la simulación deben medirse con **Release**. En Debug, el contador de memoria agrega costo a cada `malloc`/`free` y distorsiona los resultados.

### Por consola (gcc)

```bash
# Release
gcc -Wall -O2 *.c -o lru_cache

# Debug con detección de fugas
gcc -Wall -g -DCONTAR_MEM *.c -o lru_cache_dbg -Wl,--wrap=malloc -Wl,--wrap=free
```

### Menú

```
===== LRU Cache =====
1. Generar lote concentrado
2. Generar lote disperso
3. Generar lote ordenado
4. Test cruzado (Parte 1 vs Parte 2)
5. Simulacion Tweener
0. Salir
```

Flujo típico: generar los lotes (opciones 1 a 3) y después correr la simulación (opción 5). Los valores entre corchetes son los de defecto: Enter para aceptarlos.

---

## 4. Diseño de los TDAs

### 4.1. Formato de una entrada: `[clave][dato]`

Cada elemento de la caché se guarda en **un único bloque** de memoria con la clave al principio y el dato a continuación:

```
 info (un solo malloc)
┌─────────────┬────────────────────────┐
│    clave    │         dato           │
│ (tamClave)  │       (tamDato)        │
└─────────────┴────────────────────────┘
```

Esto resuelve dos cosas a la vez:

- **Genericidad:** la caché no sabe qué tipo de clave ni de dato guarda, solo copia bytes.
- **Comparación directa:** como la clave está al inicio del bloque, el comparador que da el usuario (`cmpInt`, por ejemplo) se puede aplicar directamente contra el `info` del nodo, sin adaptadores.

Dos funciones de `utilidades.c` manejan el formato:

- `crearEntrada(clave, tamClave, dato, tamDato)`: arma el bloque `[clave][dato]`.
- `copiarDatoEntrada(entrada, tamEntrada, tamClave, dato, tamDato)`: copia a `dato` solo la parte del dato.

### 4.2. TDAs básicos

Siguen el estilo de los TDAs de la cátedra (`info` y `tamInfo`, punteros a puntero, códigos de retorno), con las primitivas adicionales que la caché necesita.

**Lista simple (`lista.h`)**

| Primitiva | Descripción |
|---|---|
| `crearLista`, `vaciarLista` | Inicializar y liberar todo |
| `ponerAlComienzoLista` | Copia el dato en un nodo nuevo al inicio |
| `sacarUltimoLista` | Quita el último nodo (opcionalmente copia su dato) |
| `buscarClaveLista` | Devuelve el **enlace** (`tLista *`) al nodo con esa clave |
| `moverAlComienzoLista` | Reenlaza un nodo al inicio, sin `malloc` ni `free` |
| `eliminarNodoLista` | Quita y libera el nodo de un enlace |

**Lista doble (`listaDoble.h`)**

`tListaD` guarda los dos extremos (`ini` y `fin`) y cada nodo tiene `sig` y `ant`. Así, mover al inicio y eliminar un nodo (incluido el último) son **O(1)** y no recorren la lista.

| Primitiva | Descripción |
|---|---|
| `crearListaDoble`, `vaciarListaDoble` | Inicializar y liberar todo |
| `ponerAlInicioListaDoble` | Copia el dato en un nodo nuevo al inicio (queda en `ini`) |
| `moverAlInicioListaDoble` | Mueve un nodo al inicio en O(1) |
| `eliminarNodoListaDoble` | Quita y libera un nodo en O(1) |

**Árbol (`arbol.h`)**

Árbol binario de búsqueda, no balanceado.

| Primitiva | Descripción |
|---|---|
| `crearArbol`, `vaciarArbol` | Inicializar y liberar todo (`vaciarArbol` es iterativo, no recursivo) |
| `buscarEnlaceArbol` | Devuelve el enlace al nodo con esa clave, o el enlace `NULL` donde se insertaría |
| `insertarEnEnlaceArbol` | Crea un nodo (copia del dato) en un enlace ya encontrado |
| `eliminarEnlaceArbol` | Elimina el nodo de un enlace (con dos hijos usa el sucesor inorden) |
| `alturaArbol` | Cantidad de niveles |

**Por qué trabajan con enlaces.** Devolver el enlace (`tLista *`, `tArbol *`) en lugar del nodo evita buscar dos veces: la misma búsqueda sirve para insertar, mover o eliminar.

### 4.3. Códigos de retorno (`utilidades.h`)

| Código | Valor | Significado |
|---|---|---|
| `TODO_OK` | 0 | Operación exitosa |
| `SIN_MEM` | 1 | Falló un `malloc` |
| `NO_EXISTE` | 2 | La clave no está |
| `LISTA_VACIA` | 3 | Se pidió sacar de una lista vacía |
| `PARAM_INVALIDO` | 4 | Parámetros incorrectos |

---

## 5. Parte 1: caché con lista simple

```c
typedef struct
{
    tLista lista;          /* inicio = usado mas recientemente */
    unsigned cantidad;
    unsigned capacidad;
    unsigned tamClave;     /* todas las claves de una cache tienen el mismo tipo */
    tCmp cmp;
} tLruCacheLista;
```

La lista **es** la caché: ahí viven los datos y el orden de uso.

```
lista ─► [clave|dato] ─► [clave|dato] ─► [clave|dato] ─► NULL
         más reciente                     menos reciente
```

### API

```c
int  crearLruCacheLista (tLruCacheLista *c, unsigned capacidad, unsigned tamClave, tCmp cmp);
int  agregarLruCacheLista(tLruCacheLista *c, const void *clave, const void *dato, unsigned tamDato);
int  obtenerLruCacheLista(tLruCacheLista *c, const void *clave, void *dato, unsigned tamDato);
int  borrarLruCacheLista (tLruCacheLista *c, const void *clave);
void vaciarLruCacheLista (tLruCacheLista *c);
```

### Cómo funciona cada operación

- **obtener:** `buscarClaveLista` recorre la lista comparando claves. Si la encuentra, `moverAlComienzoLista` reenlaza el nodo al inicio y se copia el dato. Si no, devuelve `NO_EXISTE`.
- **agregar:** busca la clave. Si ya existía, elimina ese nodo. Arma la entrada `[clave][dato]`, la pone al inicio y, si `cantidad > capacidad`, quita el último con `sacarUltimoLista`.
- **borrar:** busca la clave y elimina el nodo.
- **vaciar:** libera todos los nodos y deja `cantidad = 0`.

### Costo

Como la lista no tiene índice, **cada operación recorre la lista**: O(n) con n igual a la cantidad de elementos guardados (hasta la capacidad). Ese es el problema que resuelve la Parte 2.

---

## 6. Parte 2: caché con lista doble y árbol índice

```c
typedef struct
{
    tListaD lista;         /* ini = usado mas recientemente, fin = menos */
    tArbol indice;
    unsigned cantidad;
    unsigned capacidad;
    unsigned tamClave;
    tCmp cmp;
} tLruCacheArbol;
```

Dos estructuras con trabajos distintos:

| Estructura | Rol | Qué guarda |
|---|---|---|
| **Lista doble** | **Es la caché**: datos y orden de uso | `[clave][dato]` |
| **Árbol** | **Índice**: encuentra rápido el nodo de una clave | `[clave][puntero al nodo de la lista]` |

```
                 ÁRBOL (índice, por clave)
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

Cada nodo del árbol tiene su **propia copia de la clave** (al inicio del bloque, para que el comparador funcione directo) seguida del puntero al nodo de la lista que contiene el dato real.

### API

```c
int  crearLruCacheArbol (tLruCacheArbol *c, unsigned capacidad, unsigned tamClave, tCmp cmp);
int  agregarLruCacheArbol(tLruCacheArbol *c, const void *clave, const void *dato, unsigned tamDato);
int  obtenerLruCacheArbol(tLruCacheArbol *c, const void *clave, void *dato, unsigned tamDato);
int  borrarLruCacheArbol (tLruCacheArbol *c, const void *clave);
void vaciarLruCacheArbol (tLruCacheArbol *c);
unsigned alturaLruCacheArbol(const tLruCacheArbol *c);   /* altura del árbol índice */
```

### Cómo funciona cada operación

- **obtener:** busca en el árbol, lee el puntero al nodo de la lista, lo mueve al inicio en O(1) y copia el dato.
- **agregar:** busca en el árbol. Si la clave existía, la quita de **ambas** estructuras. Después inserta la entrada en la lista y en el árbol (en el enlace ya encontrado). Si hay desborde, descarta el último.
- **descartar el último:** toma la clave del nodo `fin` de la lista, busca ese nodo en el árbol y lo quita de las dos estructuras.
- **borrar:** busca en el árbol y quita la entrada de ambas.
- **vaciar:** `vaciarArbol` y `vaciarListaDoble`, cada una libera solo lo suyo.

### Consistencia entre las dos estructuras

Toda alta y toda baja pasan por dos funciones internas (`insertarEntrada` y `quitarEntrada`), que actualizan **siempre ambas** estructuras. Así no hay forma de que una quede desactualizada respecto de la otra. Si falla la inserción en el árbol por falta de memoria, se deshace el alta en la lista.

### Costo

| Operación | Costo |
|---|---|
| Buscar por clave | O(h), con h la altura del árbol |
| Mover al inicio / eliminar un nodo de la lista | O(1) |
| Descartar el último | O(h) para sacarlo del árbol + O(1) en la lista |

Con claves en orden aleatorio, h ≈ log n. Con claves **ordenadas**, el árbol (que no se balancea) se degenera en una cadena y h ≈ n. Es el caso del lote ordenado.

---

## 7. Manejo de memoria

El requisito era que ningún dato quede sin liberar ni se libere dos veces, dado que está referenciado desde dos estructuras. La regla de diseño es de **propiedad única**:

| Memoria | Dueño | Quién la libera |
|---|---|---|
| Bloque `[clave][dato]` | Lista (simple o doble) | `vaciarLista...` / `eliminarNodoLista...` |
| Bloque `[clave][puntero]` del índice | Árbol | `vaciarArbol` / `eliminarEnlaceArbol` |
| Nodo de la lista | Lista | Idem |

- El árbol **nunca** libera el nodo de la lista: solo lo referencia.
- El árbol tiene su **propia copia de la clave**, así que no depende de memoria de la lista.
- Al eliminar un nodo del árbol con dos hijos se intercambian los datos con el sucesor y se libera el nodo sucesor. La clave y su puntero viajan juntos, por lo que nunca quedan inconsistentes.
- En `quitarEntrada`, el puntero al nodo de la lista se lee **antes** de tocar el árbol.
- Al descartar el último, la clave se lee del nodo de la lista **antes** de liberarlo.

**Cómo se verifica.** Con el target Debug (`-DCONTAR_MEM` y `--wrap`), el test cruzado cuenta cada `malloc` y `free` de los TDAs y reporta el balance. Debe dar 0 en todos los escenarios.

---

## 8. Parte 3: simulación Tweener

### 8.1. Archivos de datos

Los genera el programa (opciones 1 a 3 del menú):

| Archivo | Contenido |
|---|---|
| `posteos.dat` | 50000 registros `{id: int, texto: char[141]}`. El `id` coincide con la posición del registro, así que se lee con `fseek` directo |
| `usuarios_concentrado.dat` | Registros `{usuario: char[15], feed: int[20]}` del lote concentrado |
| `usuarios_disperso.dat` | Ídem, lote disperso |
| `usuarios_ordenado.dat` | Ídem, lote ordenado |
| `posteos.txt`, `usuarios_<tipo>.txt` | Versión legible de cada `.dat` (ver [8.4](#84-exportación-a-texto)). Solo sirven para revisar los datos: la simulación lee los `.dat` |

Cada lote tiene como mínimo 200 usuarios (hasta 99999 en los lotes concentrado y disperso). Cada usuario es un registro distinto (`usuario00001`, `usuario00002`, ...) y su feed siempre tiene exactamente 20 ids.

> `posteos.dat` se vuelve a escribir cada vez que se genera un lote. Si se generan los tres lotes con **la misma semilla**, el archivo queda idéntico en las tres corridas. Con semillas distintas los textos cambian, pero los ids de los feeds siguen siendo válidos (0 a 49999), así que la simulación funciona igual.

### 8.2. Generador de lotes

Recibe una **semilla** para `srand`, de modo que la misma semilla, tipo y parámetros reproducen exactamente el mismo lote.

| Lote | Cómo se generan los ids de los feeds | Situación que representa |
|---|---|---|
| **Concentrado** | Se eligen siempre dentro de un *pool* chico de posteos populares (por defecto 100) | Muchos usuarios ven los mismos posteos |
| **Disperso** | Cualquier posteo de los 50000 con igual probabilidad | Los usuarios ven posteos mayormente distintos |
| **Ordenado** | `id = contador × 50000 / total`, estrictamente creciente en toda la simulación | Los ids aparecen en orden creciente |

**Restricción del lote ordenado:** `usuarios × 20 ≤ 50000`, es decir, hasta 2500 usuarios, porque cada id debe ser distinto y creciente.

Cada tipo de lote es una función generadora de ids (`idConcentrado`, `idDisperso`, `idOrdenado`) registrada en una tabla. `generarLote` busca la función por nombre y el resto del código no se entera de cuál se usa, así que agregar un tipo de lote nuevo es agregar una fila a la tabla.

### 8.3. Generación del texto de los posteos

Los textos miden entre 1 y **139 caracteres** (el enunciado pide "menos de 140") y el campo `char texto[141]` siempre termina en `'\0'`: antes de generar cada posteo se hace `memset` a 0, y el texto nunca pasa de la posición 138. Ese `memset` además hace que el archivo sea reproducible byte a byte.

Hay **dos versiones** de generación, ambas en `generadorLotes.c`. Se elige una sola, cambiando una línea de `generadorLotes.h`:

```c
#define TEXTO_LETRAS     0
#define TEXTO_PALABRAS   1
#define TEXTO_ELEGIDO    TEXTO_PALABRAS     /* cambiar acá para elegir la versión */
```

`generarPosteos` toma la función de una tabla (`generadoresTexto`) usando ese índice. Como las dos funciones están en la tabla, ambas quedan referenciadas y el compilador no avisa de funciones sin usar.

| Versión | Función | Cómo arma el texto |
|---|---|---|
| **Letras al azar** | `generarTextoLetrasAleatorias` | Sortea un largo de 1 a 139 y llena cada posición con una letra o un espacio al azar |
| **Palabras al azar** (la usada en los resultados) | `generarTextoPalabrasAleatorias` | Sortea un largo objetivo de 1 a 139 y concatena palabras de una lista, separadas por un espacio |

**Versión con palabras.** La lista tiene **120 palabras** comunes (saludos, días de la semana, comida, lugares, actividades, adjetivos, verbos y conectores). Todas están en minúscula y **sin tildes ni ñ** ("manana", "dia"), para que no dependan de la codificación del archivo ni de la consola. La más larga mide 10 caracteres.

El algoritmo, con un solo ciclo `do-while` (sin `break` ni `continue`):

1. Sortea el largo objetivo (1 a 139) y la primera palabra.
2. Copia la palabra (precedida de un espacio si no es la primera).
3. Sortea la siguiente palabra y repite **solo si** `largo + 1 + largo de la palabra ≤ objetivo`.

Garantías:

- Siempre hay al menos una palabra, incluso con objetivo 1 (nunca queda un texto vacío).
- El texto nunca supera 139 caracteres, porque una palabra solo se agrega si entra en el objetivo.
- El largo real queda *cerca* del objetivo pero no exacto, porque las palabras tienen distinto tamaño. Para este TP alcanza con que sea menor a 140.

Resultan textos con apariencia de posteo ("hoy cafe con amigos feliz"), aunque sin sentido gramatical.

**Efecto en la secuencia de `rand()`.** Todo el generador comparte un único `srand`, y los feeds se generan *después* de los textos. Por eso la versión elegida cambia cuántos números aleatorios se consumen antes de los feeds:

| Versión | Llamadas a `aleatorio` por posteo (aprox.) |
|---|---|
| Letras al azar | ~70 (de 2 a 140) |
| Palabras al azar | ~12 a 15 |

Consecuencias:

- Con la **misma versión** y la misma semilla, el lote se reproduce exactamente.
- Con **versiones distintas**, la misma semilla da feeds distintos. Los resultados de la sección 10 corresponden a `TEXTO_PALABRAS`; si se cambia la versión hay que volver a generar los lotes y a correr la simulación.

### 8.4. Exportación a texto

Después de escribir los `.dat`, `generarLote` exporta cada uno a un `.txt` con el mismo nombre:

```c
ok = generarPosteos(NOMBRE_POSTEOS)
     && generarUsuarios(nombreUsu, usuarios, generar, &ctx)
     && exportarATexto(NOMBRE_POSTEOS, sizeof(tPosteo), formatearPosteo) == TODO_OK
     && exportarATexto(nombreUsu, sizeof(tUsuario), formatearUsuario) == TODO_OK;
```

| Función | Rol |
|---|---|
| `exportarATexto(nombreBin, tamRegistro, formatear)` | Genérica: lee el binario registro por registro y le pasa cada uno a `formatear`. El `.txt` se llama como el binario con la extensión cambiada (`posteos.dat` → `posteos.txt`) |
| `formatearPosteo` | Escribe una línea por posteo con el formato `id: texto` |
| `formatearUsuario` | Escribe una línea por usuario con el formato `usuario: id id id ...` (los 20 ids del feed) |

`exportarATexto` no conoce el tipo de registro: solo sabe su tamaño y delega el formato en la función que recibe (`tFormatear`). Así sirve para cualquier archivo de registros de tamaño fijo y no se duplica código entre posteos y usuarios. Devuelve `TODO_OK`, `SIN_MEM` o `PARAM_INVALIDO`, y cierra los archivos y libera el buffer en todos los casos.

### 8.5. Qué hace la simulación

Para **cada usuario**, busca cada uno de los 20 posteos de su feed. Los datos leídos **no se muestran, no se imprimen ni se guardan**: solo se mide.

```
para cada posteo del feed:
    obtener de la caché
    ├─ si está  ──► ACIERTO (no se toca el archivo)
    └─ si no    ──► FALLO: leer posteos.dat (fseek + fread) y agregarlo a la caché
```

Se corren tres variantes sobre cada lote:

| Variante | Qué es |
|---|---|
| `sin cache` | Siempre lee del archivo (línea base) |
| `parte 1` | Con `tLruCacheLista` |
| `parte 2` | Con `tLruCacheArbol` (informa además la altura del árbol) |

Cada variante con caché se ejecuta con capacidades **10, 100, 1000 y 10000**.

### 8.6. Columnas de la tabla

| Columna | Significado |
|---|---|
| `variante` | `sin cache`, `parte 1` o `parte 2` |
| `capacid.` | Capacidad de la caché |
| `total(ms)` | Tiempo de **una** simulación completa (promedio de las repeticiones) |
| `us/usuario` | Tiempo total dividido por la cantidad de usuarios |
| `us/posteo` | Tiempo total dividido por (usuarios × 20) |
| `aciertos` | Posteos encontrados en la caché |
| `fallos` | Posteos no encontrados en la caché |
| `lecturas` | Lecturas realizadas a `posteos.dat` |
| `altura` | Altura del árbol índice (solo Parte 2) |

**Relaciones que siempre se cumplen:**

- `aciertos + fallos = usuarios × 20`
- `lecturas = fallos` (cada fallo hace exactamente una lectura al archivo)
- `sin cache`: todo son fallos
- Los **aciertos son idénticos** en Parte 1 y Parte 2 para la misma capacidad, porque ambas aplican la misma política LRU

### 8.7. Cómo se miden los tiempos

El tiempo de leer **un** posteo es menor que la resolución del reloj, así que no se mide individualmente: se mide el tiempo total de la simulación y los tiempos por posteo y por usuario se calculan como promedio, como pide el enunciado.

Para obtener un valor estable, la simulación completa se **repite N veces** (parámetro "Repeticiones", por defecto 10), cada vez con una caché nueva, y se divide el tiempo total por N. Los contadores no se promedian: son iguales en todas las repeticiones, porque el lote es el mismo y LRU es determinista.

La medición usa `clock()`, cuya resolución depende del sistema (en Windows es de aproximadamente 1 ms). Por eso los tiempos totales salen en **múltiplos de 1 ms / N**: con 10 repeticiones, en saltos de 0,1 ms. Las diferencias de una décima entre dos filas no son significativas; las tendencias y los contadores sí.

### 8.8. Funciones de `simuladorTweener.c`

| Función | Rol |
|---|---|
| `ejecutarSimulacion` | Corre todas las variantes y capacidades sobre un lote e imprime la tabla |
| `medirYMostrarFila` | Una fila de la tabla: repite el procesamiento N veces, promedia el tiempo e imprime |
| `realizarSimulacion` | Una simulación completa con caché nueva: recorre todos los usuarios y cuenta aciertos/fallos/lecturas |
| `procesarUsuario` | Procesa los 20 posteos del feed de un usuario |
| `leerPosteo` | Lectura directa del archivo por posición |

Las tres variantes se manejan con la misma interfaz mediante una tabla de funciones (`tEstrategia`), de modo que el código de la simulación no se duplica para cada variante.

---

## 9. Test cruzado

La opción 4 del menú compara la Parte 1 contra la Parte 2 con **20000 operaciones aleatorias** (agregar, obtener y borrar) aplicadas por igual a ambas, con distintas capacidades (1, 2, 5, 50 y 200) y rangos de claves (igual a la capacidad y el triple).

Después de **cada operación** verifica:

- Ambas devuelven el mismo resultado y el mismo dato.
- Ambas tienen la misma cantidad de elementos y no superan la capacidad.
- Las dos listas tienen **exactamente las mismas entradas en el mismo orden**, y los enlaces `ant`/`fin` de la lista doble son correctos.
- El árbol es un BST válido y tiene tantos nodos como elementos.
- Cada nodo de la lista está en el árbol y es apuntado por su entrada del índice.

Al final, tras `vaciar`, comprueba que no quedaron restos y, con Debug, que el balance de `malloc`/`free` es **0**.

Salida obtenida (target Debug):

```
capacidad  rango    resultado  fugas
1          1        OK         0
1          3        OK         0
2          2        OK         0
2          6        OK         0
5          5        OK         0
5          15       OK         0
50         50       OK         0
50         150      OK         0
200        200      OK         0
200        600      OK         0

TODO OK
```

---

## 10. Resultados de ejemplo y análisis

Corrida en **Release** con textos de **palabras al azar** (`TEXTO_PALABRAS`), 200 usuarios, semilla 42 y 10 repeticiones. Los tiempos dependen de la máquina; lo importante son las **tendencias** y los **contadores**. Recordar que los tiempos tienen una resolución de 0,1 ms (ver [8.7](#87-cómo-se-miden-los-tiempos)).

### Lote concentrado (pool de 100)

```
variante   capacid.  total(ms)   us/usuario   us/posteo  aciertos  fallos  lecturas altura
sin cache         -      12.70        63.50       3.175         0    4000      4000      -
parte 1          10      11.90        59.50       2.975       385    3615      3615      -
parte 2          10      12.60        63.00       3.150       385    3615      3615      6
parte 1         100       0.80         4.00       0.200      3900     100       100      -
parte 2         100       0.70         3.50       0.175      3900     100       100     13
parte 1        1000       0.80         4.00       0.200      3900     100       100      -
parte 2        1000       0.60         3.00       0.150      3900     100       100     13
parte 1       10000       0.90         4.50       0.225      3900     100       100      -
parte 2       10000       0.60         3.00       0.150      3900     100       100     13
```

Con capacidad >= 100 (el tamaño del pool) casi todo son aciertos: solo 100 fallos, uno por cada posteo popular. El tiempo baja de 12,70 ms a unos 0,8 ms con la Parte 1 (unas 16 veces menos) y a 0,6 o 0,7 ms con la Parte 2 (18 a 21 veces menos). Con capacidad 10 (menor que el pool) solo el 9,6 % de los accesos son aciertos y el tiempo casi no cambia respecto de no usar caché.

Con capacidades de 1000 y 10000 los tiempos no mejoran respecto de 100: la caché solo guarda los 100 posteos distintos que existen, así que la lista nunca crece más allá de eso.

### Lote disperso

```
variante   capacid.  total(ms)   us/usuario   us/posteo  aciertos  fallos  lecturas altura
sin cache         -      13.50        67.50       3.375         0    4000      4000      -
parte 1          10      14.60        73.00       3.650         1    3999      3999      -
parte 2          10      15.20        76.00       3.800         1    3999      3999      5
parte 1         100      17.50        87.50       4.375         5    3995      3995      -
parte 2         100      16.50        82.50       4.125         5    3995      3995     14
parte 1        1000      39.80       199.00       9.950        56    3944      3944      -
parte 2        1000      16.40        82.00       4.100        56    3944      3944     20
parte 1       10000      56.70       283.50      14.175       152    3848      3848      -
parte 2       10000      16.50        82.50       4.125       152    3848      3848     23
```

Con 4000 accesos al azar sobre 50000 posteos casi nada se repite: aun con capacidad 10000 hay 152 aciertos (3,8 %). **Ninguna variante con caché le gana a no usarla.** La Parte 1 empeora a medida que crece la capacidad (14,6 → 17,5 → 39,8 → 56,7 ms), mientras que la Parte 2 queda casi plana alrededor de 16,5 ms, unos 3 ms por encima de la línea base: ese es el costo fijo de mantener el índice y la lista sin obtener aciertos a cambio.

### Lote ordenado

```
variante   capacid.  total(ms)   us/usuario   us/posteo  aciertos  fallos  lecturas altura
sin cache         -       9.30        46.50       2.325         0    4000      4000      -
parte 1          10      10.00        50.00       2.500         0    4000      4000      -
parte 2          10      10.70        53.50       2.675         0    4000      4000     10
parte 1         100      12.80        64.00       3.200         0    4000      4000      -
parte 2         100      13.00        65.00       3.250         0    4000      4000    100
parte 1        1000      35.50       177.50       8.875         0    4000      4000      -
parte 2        1000      35.60       178.00       8.900         0    4000      4000   1000
parte 1       10000      53.00       265.00      13.250         0    4000      4000      -
parte 2       10000      66.00       330.00      16.500         0    4000      4000   4000
```

Sin repeticiones no hay aciertos, así que la caché solo agrega costo. El árbol queda degenerado: su altura es igual a la capacidad (10, 100, 1000) y con capacidad 10000 llega a 4000, es decir, a la cantidad total de posteos consultados, porque nunca se llena y no descarta nada.

### Comparación con no usar caché

Tiempo de cada variante dividido por el de `sin cache` (menor que 1 significa mejora):

| Lote | Capacidad | Parte 1 | Parte 2 |
|---|---|---|---|
| Concentrado | 10 | 0,94 | 0,99 |
| Concentrado | 100 | 0,06 | 0,06 |
| Concentrado | 1000 | 0,06 | 0,05 |
| Concentrado | 10000 | 0,07 | 0,05 |
| Disperso | 10 | 1,08 | 1,13 |
| Disperso | 100 | 1,30 | 1,22 |
| Disperso | 1000 | 2,95 | 1,21 |
| Disperso | 10000 | 4,20 | 1,22 |
| Ordenado | 10 | 1,08 | 1,15 |
| Ordenado | 100 | 1,38 | 1,40 |
| Ordenado | 1000 | 3,82 | 3,83 |
| Ordenado | 10000 | 5,70 | 7,10 |

### Respuestas a las preguntas del enunciado

**1. ¿En qué situaciones el uso de la caché mejora el tiempo de procesamiento y en cuáles no?**

Mejora cuando hay **localidad** y la capacidad alcanza para el conjunto de posteos que se repiten: en el lote concentrado con capacidad >= 100 el tiempo cae de 12,70 ms a 0,6 o 0,8 ms (hasta 21 veces menos), con 3900 aciertos sobre 4000 accesos. Si la capacidad es menor que el conjunto popular (capacidad 10 contra un pool de 100) los aciertos bajan al 9,6 % y la mejora desaparece.

No mejora, y hasta empeora, cuando casi no hay repeticiones: en el disperso (3,8 % de aciertos como máximo) y en el ordenado (0 aciertos). La caché agrega trabajo (buscar, insertar, descartar) sin ahorrar lecturas. Además, en esta simulación leer del archivo es barato (alrededor de 2,3 a 3,4 µs por posteo) porque el sistema operativo mantiene `posteos.dat` en su propia caché, de modo que cada acierto ahorra poco y hace falta una tasa de aciertos muy alta para compensar el costo de mantener la caché.

También se nota que la línea base cambia entre lotes (12,70 / 13,50 / 9,30 ms): el lote ordenado lee el archivo en orden creciente, lo que favorece la lectura secuencial del sistema operativo, y por eso es el que menos tarda sin caché.

**2. ¿A partir de qué capacidad deja de convenir la implementación de la Parte 1? ¿Por qué?**

No hay una capacidad única: depende de cuántos aciertos compensen el recorrido de la lista.

- En el **concentrado** no deja de convenir en ningún valor probado (0,90 ms con capacidad 10000 contra 12,70 sin caché), porque el costo depende de cuántos elementos hay *guardados*, no de la capacidad declarada: la lista nunca pasa de 100 entradas.
- En el **disperso** y en el **ordenado** no conviene en ninguna capacidad probada. Ya con capacidad 10 es un 8 % más lenta que no usar caché, con 100 es entre un 30 % y un 38 % más lenta, y desde **1000** el costo se dispara: casi 3 veces más lenta en el disperso y casi 4 en el ordenado, hasta 4,2 y 5,7 veces con capacidad 10000.

La causa es que cada `obtener` fallido recorre la lista completa, y luego `agregar` la recorre de nuevo para buscar un duplicado y, cuando la caché está llena, otra vez para sacar el último. Con pocos aciertos todo ese recorrido se paga y no se ahorra nada: el costo por posteo crece linealmente con la cantidad de elementos guardados, hasta superar por mucho al de leer directamente el archivo.

**3. ¿Qué ocurre con el árbol índice en el lote ordenado y cómo afecta al rendimiento?**

Las claves llegan en orden creciente, así que cada inserción va al extremo derecho del árbol, que no se balancea, y este se convierte en una **lista enlazada disfrazada**: su altura es igual a la cantidad de elementos guardados (10, 100 y 1000 con esas capacidades, y 4000 con capacidad 10000, donde no se descarta nada). En el disperso, en cambio, la altura es de 5 a 23 niveles, cercana al logaritmo de la cantidad de claves.

Buscar pasa de O(log n) a O(n), y la Parte 2 pierde su ventaja: en este lote queda **igual que la Parte 1** hasta capacidad 1000 (13,0 contra 12,8 ms y 35,6 contra 35,5 ms) y **peor** con capacidad 10000 (66,0 contra 53,0 ms, un 25 % más lenta), donde además mantiene el índice y la lista y hace más reservas de memoria. Frente a no usar caché es peor en todas las capacidades. Esa diferencia con el disperso, donde la Parte 2 queda plana en torno a 16,5 ms, muestra que el árbol solo ayuda si las claves llegan sin orden.

Una posible razón de que no pierda contra la Parte 1 con capacidades chicas es que el elemento más viejo es siempre el de menor clave, que queda en la raíz del árbol degenerado, y descartarlo no requiere recorrerlo; la Parte 1, en cambio, recorre toda la lista para sacar el último. Esa ventaja compensa el resto hasta que la caché es tan grande que nunca descarta nada.

La solución sería usar un árbol balanceado (AVL o rojinegro), que mantiene la altura en O(log n) sin importar el orden de llegada; queda fuera del alcance de este TP.

---

## 11. Decisiones de diseño y limitaciones

**`crear` recibe `tamClave`.** El enunciado habla de capacidad y comparador. Se agregó el tamaño de la clave porque el comparador es propio de cada caché y por lo tanto el tipo de clave es fijo por caché. A cambio, `agregar` y `obtener` no necesitan recibirlo en cada llamada.

**Nombres de primitivas.** Los nombres del enunciado (`crear_lrucache`, etc.) se pasaron a camelCase en español por convención del proyecto: `crearLruCacheLista`, `agregarLruCacheArbol`, y así. Se usan sufijos (`Lista`, `Arbol`) para poder tener ambas implementaciones en el mismo programa.

**La lista doble guarda `ini` y `fin`.** Es necesario para que mover al inicio y descartar el último sean O(1), como exige la Parte 2.

**Dos versiones de texto, una sola activa.** Se conservan las dos para poder compararlas, pero se elige una en un único punto (`TEXTO_ELEGIDO`) y se registran en una tabla de funciones, sin código duplicado. Los resultados de este README usan palabras.

**Textos sin tildes ni ñ.** Evita que el contenido dependa de la codificación del archivo fuente y de la consola.

**Exportación genérica.** `exportarATexto` recibe el tamaño del registro y una función de formato, en lugar de tener una versión para posteos y otra para usuarios.

**Limitaciones conocidas:**

- La clave debe ser un valor de **tamaño fijo** (`int`, estructura, `char[N]`). Una clave `char *` solo copiaría el puntero, no el texto.
- Si falta memoria al **reemplazar** una clave existente, la entrada vieja ya fue quitada.
- `obtener` copia como máximo `tamDato` bytes. El llamador debe saber el tamaño de su dato.
- El árbol no se balancea (se muestra en el lote ordenado).
- `alturaArbol` es recursiva: con un árbol degenerado de decenas de miles de nodos podría agotar la pila. `vaciarArbol` es iterativo por esa razón.
- Las pruebas usan claves `int`; la genericidad con otros tipos de clave no está cubierta por el test.
- Los lotes comparten `posteos.dat`, que se reescribe en cada generación.
- Con 200 usuarios cada simulación dura pocos milisegundos y la resolución de `clock()` limita la precisión. Para diferencias finas conviene aumentar los usuarios o las repeticiones.
- Las tres columnas de tiempo salen de una sola medición total dividida por la cantidad de usuarios o de posteos: no se mide el tiempo de cada operación por separado.
