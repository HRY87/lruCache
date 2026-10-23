#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "simuladorTweener.h"
#include "registros.h"
#include "lruCacheLista.h"
#include "lruCacheArbol.h"

typedef union
{
    tLruCacheLista lista;
    tLruCacheArbol arbol;
} tCache;

/* Cada variante se maneja con las mismas cuatro operaciones */
typedef struct
{
    const char *nombre;
    int      (*crear)(tCache *c, unsigned capacidad);
    int      (*obtener)(tCache *c, const void *clave, void *dato, unsigned tamDato);
    int      (*agregar)(tCache *c, const void *clave, const void *dato, unsigned tamDato);
    void     (*vaciar)(tCache *c);
    unsigned (*altura)(const tCache *c);            /* NULL si no aplica */
} tEstrategia;

typedef struct
{
    unsigned long aciertos;
    unsigned long fallos;
    unsigned long lecturas;
    unsigned altura;
} tContadores;

/* ---------- adaptadores: sin cache (siempre falla y no guarda nada) ---------- */

static int scCrear(tCache *c, unsigned cap)
{
    (void)c; (void)cap;
    return TODO_OK;
}

static int scObtener(tCache *c, const void *clave, void *dato, unsigned tam)
{
    (void)c; (void)clave; (void)dato; (void)tam;
    return NO_EXISTE;
}

static int scAgregar(tCache *c, const void *clave, const void *dato, unsigned tam)
{
    (void)c; (void)clave; (void)dato; (void)tam;
    return TODO_OK;
}

static void scVaciar(tCache *c)
{
    (void)c;
}

/* ---------- adaptadores: Parte 1 ---------- */

static int lCrear(tCache *c, unsigned cap)
{
    return crearLruCacheLista(&c->lista, cap, sizeof(int), cmpInt);
}

static int lObtener(tCache *c, const void *clave, void *dato, unsigned tam)
{
    return obtenerLruCacheLista(&c->lista, clave, dato, tam);
}

static int lAgregar(tCache *c, const void *clave, const void *dato, unsigned tam)
{
    return agregarLruCacheLista(&c->lista, clave, dato, tam);
}

static void lVaciar(tCache *c)
{
    vaciarLruCacheLista(&c->lista);
}

/* ---------- adaptadores: Parte 2 ---------- */

static int aCrear(tCache *c, unsigned cap)
{
    return crearLruCacheArbol(&c->arbol, cap, sizeof(int), cmpInt);
}

static int aObtener(tCache *c, const void *clave, void *dato, unsigned tam)
{
    return obtenerLruCacheArbol(&c->arbol, clave, dato, tam);
}

static int aAgregar(tCache *c, const void *clave, const void *dato, unsigned tam)
{
    return agregarLruCacheArbol(&c->arbol, clave, dato, tam);
}

static void aVaciar(tCache *c)
{
    vaciarLruCacheArbol(&c->arbol);
}

static unsigned aAltura(const tCache *c)
{
    return alturaLruCacheArbol(&c->arbol);
}

static const tEstrategia estrategias[] =
{
    {"sin cache", scCrear, scObtener, scAgregar, scVaciar, NULL},
    {"parte 1",   lCrear,  lObtener,  lAgregar,  lVaciar,  NULL},
    {"parte 2",   aCrear,  aObtener,  aAgregar,  aVaciar,  aAltura}
};

/* Lee todo el archivo de usuarios en memoria; NULL si falla */
static tUsuario *cargarUsuarios(const char *nombre, size_t *cant)
{
    FILE *pf = fopen(nombre, "rb");
    tUsuario *usu = NULL;
    long bytes;

    if(!pf)
        return NULL;

    fseek(pf, 0, SEEK_END);
    bytes = ftell(pf);
    rewind(pf);

    *cant = (size_t)bytes / sizeof(tUsuario);

    if(*cant > 0)
        usu = malloc(*cant * sizeof(tUsuario));

    if(usu && fread(usu, sizeof(tUsuario), *cant, pf) != *cant)
    {
        free(usu);
        usu = NULL;
    }

    fclose(pf);

    return usu;
}

/* Lectura directa: el id coincide con la posicion del registro */
static int leerPosteo(FILE *pf, int id, tPosteo *post)
{
    return fseek(pf, (long)id * (long)sizeof(tPosteo), SEEK_SET) == 0
        && fread(post, sizeof(*post), 1, pf) == 1
        && post->id == id;
}

/* Busca cada posteo del feed en la cache y, si falla, en el archivo (y lo agrega a la cache) */
static int procesarUsuario(const tEstrategia *e, tCache *c, FILE *pf, const tUsuario *usu, tContadores *k)
{
    const int *p = usu->feed;
    const int *fin = usu->feed + LARGO_FEED;
    tPosteo post;
    int ok = 1;

    while(p < fin && ok)
    {
        if(e->obtener(c, p, &post, sizeof(post)) == TODO_OK)
            k->aciertos++;
        else
        {
            k->fallos++;
            k->lecturas++;
            ok = leerPosteo(pf, *p, &post)
                 && e->agregar(c, p, &post, sizeof(post)) == TODO_OK;
        }
        p++;
    }

    return ok;
}

/* Una corrida completa con una cache nueva. Los datos leidos se descartan. */
static int correrUna(const tEstrategia *e, unsigned capacidad, FILE *pf,
                     const tUsuario *usu, size_t cant, tContadores *k)
{
    tCache c;
    const tUsuario *u = usu;
    const tUsuario *fin = usu + cant;
    int ok = e->crear(&c, capacidad) == TODO_OK;

    k->aciertos = 0;
    k->fallos = 0;
    k->lecturas = 0;
    k->altura = 0;

    if(!ok)
        return 0;

    while(u < fin && ok)
    {
        ok = procesarUsuario(e, &c, pf, u, k);
        u++;
    }

    if(e->altura)
        k->altura = e->altura(&c);

    e->vaciar(&c);

    return ok;
}

/* Repite la corrida, promedia los tiempos e imprime una fila de la tabla */
static int correrFila(const tEstrategia *e, unsigned capacidad, int repeticiones,
                      FILE *pf, const tUsuario *usu, size_t cant)
{
    tContadores k;
    clock_t ini = clock();
    double ms;
    int i;
    int ok = 1;

    for(i = 0; i < repeticiones && ok; i++)
        ok = correrUna(e, capacidad, pf, usu, cant, &k);

    ms = (double)(clock() - ini) * 1000.0 / CLOCKS_PER_SEC / repeticiones;

    if(!ok)
    {
        printf("%-10s error durante la simulacion\n", e->nombre);
        return 0;
    }

    printf("%-10s ", e->nombre);

    if(capacidad)
        printf("%8u ", capacidad);
    else
        printf("%8s ", "-");

    printf("%10.2f %12.2f %11.3f %9lu %7lu %9lu ",
           ms,
           ms * 1000.0 / (double)cant,
           ms * 1000.0 / ((double)cant * LARGO_FEED),
           k.aciertos, k.fallos, k.lecturas);

    if(e->altura)
        printf("%6u\n", k.altura);
    else
        printf("%6s\n", "-");

    return 1;
}

/* Corre todas las variantes y capacidades sobre un lote y muestra la tabla */
int ejecutarSimulacion(const char *tipo, int repeticiones)
{
    static const unsigned capacidades[] = {10, 100, 1000, 10000};
    const unsigned *pc;
    const unsigned *finC = capacidades + sizeof(capacidades) / sizeof(*capacidades);
    const tEstrategia *pe;
    const tEstrategia *finE = estrategias + sizeof(estrategias) / sizeof(*estrategias);
    char nombre[64];
    size_t cant = 0;
    tUsuario *usu;
    FILE *pf;
    int ok;

    if(repeticiones < 1)
        repeticiones = 1;

    snprintf(nombre, sizeof(nombre), FORMATO_USUARIOS, tipo);

    usu = cargarUsuarios(nombre, &cant);
    if(!usu)
    {
        printf("No se pudo leer %s (generalo con las opciones 1 a 3)\n", nombre);
        return PARAM_INVALIDO;
    }

    pf = fopen(NOMBRE_POSTEOS, "rb");
    if(!pf)
    {
        printf("No se pudo abrir %s\n", NOMBRE_POSTEOS);
        free(usu);
        return PARAM_INVALIDO;
    }

    printf("\n--- Lote %s: %u usuarios x %d posteos, %d repeticiones ---\n",
           tipo, (unsigned)cant, LARGO_FEED, repeticiones);
    printf("%-10s %8s %10s %12s %11s %9s %7s %9s %6s\n",
           "variante", "capacid.", "total(ms)", "us/usuario", "us/posteo",
           "aciertos", "fallos", "lecturas", "altura");

    ok = correrFila(estrategias, 0, repeticiones, pf, usu, cant);   /* sin cache */

    for(pc = capacidades; pc < finC && ok; pc++)
    {
        for(pe = estrategias + 1; pe < finE && ok; pe++)
            ok = correrFila(pe, *pc, repeticiones, pf, usu, cant);
    }

    fclose(pf);
    free(usu);

    return ok ? TODO_OK : PARAM_INVALIDO;
}
