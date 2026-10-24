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
├── lru_cache.cbp          Proyecto Code::Blocks (Debug y Release)
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
├── generador.h/.c         Generador de lotes de prueba (con semilla)
├── simulador.h/.c         Simulación Tweener y tabla de resultados
├── testCruzado.h/.c       Comparación Parte 1 vs Parte 2 y detección de fugas
└── main.c                 Menú principal
```

Separación de responsabilidades en tres capas:

| Capa | Archivos | Responsabilidad |
|---|---|---|
| TDAs básicos | `lista`, `listaDoble`, `arbol` | Estructuras de datos genéricas, sin saber nada de cachés |
| TDAs de caché | `lruCacheLista`, `lruCacheArbol` | Solo la lógica LRU, apoyada en los TDAs básicos |
| Aplicación | `generador`, `simulador`, `testCruzado`, `main` | Parte 3 y pruebas |

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

Los textos tienen entre 1 y 139 caracteres (menos de 140, como pide el enunciado). Cada lote tiene como mínimo 200 usuarios.

### 8.2. Generador de lotes

Recibe una **semilla** para `srand`, de modo que la misma semilla, tipo y parámetros reproducen exactamente el mismo lote.

| Lote | Cómo se generan los ids de los feeds | Situación que representa |
|---|---|---|
| **Concentrado** | Se eligen siempre dentro de un *pool* chico de posteos populares (por defecto 100) | Muchos usuarios ven los mismos posteos |
| **Disperso** | Cualquier posteo de los 50000 con igual probabilidad | Los usuarios ven posteos mayormente distintos |
| **Ordenado** | `id = contador × 50000 / total`, estrictamente creciente en toda la simulación | Los ids aparecen en orden creciente |

**Restricción del lote ordenado:** `usuarios × 20 ≤ 50000`, es decir, hasta 2500 usuarios, porque cada id debe ser distinto y creciente.

### 8.3. Qué hace la simulación

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

### 8.4. Columnas de la tabla

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

### 8.5. Cómo se miden los tiempos

El tiempo de leer **un** posteo es menor que la resolución del reloj, así que no se mide individualmente: se mide el tiempo total de la simulación y los tiempos por posteo y por usuario se calculan como promedio, como pide el enunciado.

Para obtener un valor estable, la simulación completa se **repite N veces** (parámetro "Repeticiones", por defecto 10), cada vez con una caché nueva, y se divide el tiempo total por N. Los contadores no se promedian: son iguales en todas las repeticiones, porque el lote es el mismo y LRU es determinista.

### 8.6. Funciones de `simulador.c`

| Función | Rol |
|---|---|
| `ejecutarSimulacion` | Corre todas las variantes y capacidades sobre un lote e imprime la tabla |
| `medirYMostrarFila` | Una fila de la tabla: repite el procesamiento N veces, promedia el tiempo e imprime |
| `procesarTodosLosUsuarios` | Una simulación completa con caché nueva: recorre todos los usuarios y cuenta aciertos/fallos/lecturas |
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

Salida esperada:

```
capacidad  rango    resultado  fugas
1          1        OK         0
...
TODO OK
```

---

## 10. Resultados de ejemplo y análisis

Corrida de ejemplo en Release con 200 usuarios, semilla 42 y 3 repeticiones (los tiempos dependen de la máquina; lo importante son las **tendencias** y los **contadores**).

### Lote concentrado (pool de 100)

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

Con capacidad >= 100 (el tamaño del pool) casi todo son aciertos: solo 100 fallos, uno por cada posteo popular. El tiempo baja unas 3 veces con la Parte 1 y unas 5 con la Parte 2. Con capacidad 10 (menor que el pool) hay pocos aciertos y la caché todavía no compensa su costo.

### Lote disperso

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

Con 4000 accesos al azar sobre 50000 posteos casi nada se repite: los aciertos son marginales. Ninguna variante con caché le gana a no usarla. La Parte 1 empeora a medida que crece la capacidad, mientras que la Parte 2 queda casi plana.

### Lote ordenado

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

Sin repeticiones no hay aciertos, y el árbol queda degenerado: su altura es igual a la capacidad (hasta llenarse) y la Parte 2 resulta más lenta que la Parte 1.

### Respuestas a las preguntas del enunciado

**1. ¿En qué situaciones la caché mejora el tiempo y en cuáles no?**
Mejora cuando hay **localidad**: los mismos posteos se piden varias veces mientras siguen en la caché (lote concentrado con capacidad suficiente para el conjunto popular). No mejora, y hasta empeora, cuando casi no hay repeticiones (disperso, ordenado): la caché agrega costo (buscar, insertar, descartar) sin ahorrar lecturas. Además, en este experimento leer del archivo es muy barato (del orden de 0,4 a 3,5 µs por posteo según la máquina) porque el sistema operativo mantiene `posteos.dat` en su propia caché, de modo que cada acierto ahorra poco.

**2. ¿A partir de qué capacidad deja de convenir la Parte 1 y por qué?**
Depende del lote. En el **disperso** ya no conviene desde capacidad 100 (4,73 ms contra 2,58 ms sin caché), y desde 1000 la diferencia es grande (37,5 ms). La causa es que cada `obtener`, `agregar` y descarte recorre la lista completa: el costo por operación crece linealmente con la capacidad, mientras que los aciertos casi no aumentan. En el **concentrado** sigue conviniendo porque los aciertos compensan el recorrido, y la Parte 2 le gana de todos modos.

**3. ¿Qué ocurre con el árbol índice en el lote ordenado y cómo afecta el rendimiento?**
Las claves llegan en orden creciente, así que cada inserción va al extremo derecho del árbol y este se convierte en una **lista enlazada disfrazada**: altura igual a la cantidad de elementos (1000 con capacidad 1000, 4000 con capacidad 10000). Buscar pasa de O(log n) a O(n), y desde capacidad 100 la Parte 2 queda **más lenta que la Parte 1** (por ejemplo 80 ms contra 40 ms con capacidad 1000), porque además de recorrer el árbol mantiene la lista doble y hace más reservas de memoria. El árbol no se balancea, por eso la solución sería usar un AVL (fuera del alcance de este TP).

---

## 11. Decisiones de diseño y limitaciones

**`crear` recibe `tamClave`.** El enunciado habla de capacidad y comparador. Se agregó el tamaño de la clave porque el comparador es propio de cada caché y por lo tanto el tipo de clave es fijo por caché. A cambio, `agregar` y `obtener` no necesitan recibirlo en cada llamada.

**Nombres de primitivas.** Los nombres del enunciado (`crear_lrucache`, etc.) se pasaron a camelCase en español por convención del proyecto: `crearLruCacheLista`, `agregarLruCacheArbol`, y así. Se usan sufijos (`Lista`, `Arbol`) para poder tener ambas implementaciones en el mismo programa.

**La lista doble guarda `ini` y `fin`.** Es necesario para que mover al inicio y descartar el último sean O(1), como exige la Parte 2.

**Limitaciones conocidas:**

- La clave debe ser un valor de **tamaño fijo** (`int`, estructura, `char[N]`). Una clave `char *` solo copiaría el puntero, no el texto.
- Si falta memoria al **reemplazar** una clave existente, la entrada vieja ya fue quitada.
- `obtener` copia como máximo `tamDato` bytes. El llamador debe saber el tamaño de su dato.
- El árbol no se balancea (se muestra en el lote ordenado).
- `alturaArbol` es recursiva: con un árbol degenerado de decenas de miles de nodos podría agotar la pila. `vaciarArbol` es iterativo por esa razón.
- Las pruebas usan claves `int`; la genericidad con otros tipos de clave no está cubierta por el test.
