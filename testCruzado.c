#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lruCacheLista.h"
#include "lruCacheArbol.h"
#include "testCruzado.h"

/* ---------- contador de memoria (solo con -DCONTAR_MEM y --wrap) ---------- */

#ifdef CONTAR_MEM
static long asignaciones = 0;
static long liberaciones = 0;

void *__real_malloc(size_t n);
void __real_free(void *p);

void *__wrap_malloc(size_t n)
{
    void *p = __real_malloc(n);

    if(p)
        asignaciones++;

    return p;
}

void __wrap_free(void *p)
{
    if(p)
        liberaciones++;

    __real_free(p);
}

#define BALANCE_MEM() (asignaciones - liberaciones)
#else
#define BALANCE_MEM() 0L
#endif

/* ---------- verificaciones ---------- */

/* Misma entrada (clave y dato) en el mismo orden, y enlaces ant/fin correctos */
static int listasIguales(const tLruCacheLista *a, const tLruCacheArbol *b)
{
    const tNodo *x = a->lista;
    const tNodoD *y = b->lista.ini;
    const tNodoD *ant = NULL;

    while(x && y && y->ant == ant
          && x->tamInfo == y->tamInfo
          && memcmp(x->info, y->info, x->tamInfo) == 0)
    {
        x = x->sig;
        ant = y;
        y = y->sig;
    }

    return !x && !y && ant == b->lista.fin;
}

static unsigned contarArbol(const tNodoA *n)
{
    if(!n)
        return 0;

    return 1 + contarArbol(n->izq) + contarArbol(n->der);
}

/* min y max son cotas exclusivas (NULL = sin cota) */
static int arbolValido(const tNodoA *n, const void *min, const void *max, tCmp cmp)
{
    if(!n)
        return 1;

    return (!min || cmp(n->dato, min) > 0)
        && (!max || cmp(n->dato, max) < 0)
        && arbolValido(n->izq, min, n->dato, cmp)
        && arbolValido(n->der, n->dato, max, cmp);
}

/* Cada nodo de la lista debe estar en el arbol, apuntado desde su entrada */
static int listaEnArbol(tLruCacheArbol *b)
{
    const tNodoD *n = b->lista.ini;
    tArbol *pos;
    tNodoD *enIndice;
    int ok = 1;

    while(n && ok)
    {
        pos = buscarEnlaceArbol(&b->indice, n->info, b->cmp);
        ok = *pos != NULL;

        if(ok)
        {
            memcpy(&enIndice, (char *)(*pos)->dato + b->tamClave, sizeof(enIndice));
            ok = enIndice == n;
        }

        n = n->sig;
    }

    return ok;
}

static int consistentes(const tLruCacheLista *a, tLruCacheArbol *b)
{
    return a->cantidad == b->cantidad
        && b->cantidad <= b->capacidad
        && listasIguales(a, b)
        && contarArbol(b->indice) == b->cantidad
        && arbolValido(b->indice, NULL, NULL, b->cmp)
        && listaEnArbol(b);
}

/* ---------- escenario ---------- */

/* Operaciones aleatorias iguales sobre ambas caches. Devuelve 0 si todo OK o
   el numero de la operacion que fallo (desde 1). fuga = balance de memoria. */
static int ejecutarEscenario(unsigned capacidad, int rango, unsigned semilla, int operaciones, long *fuga)
{
    tLruCacheLista a;
    tLruCacheArbol b;
    long antes = BALANCE_MEM();
    int i, op, k, v, va, vb, resA, resB;
    int fallo = 0;

    srand(semilla);
    crearLruCacheLista(&a, capacidad, sizeof(int), cmpInt);
    crearLruCacheArbol(&b, capacidad, sizeof(int), cmpInt);

    for(i = 1; i <= operaciones && !fallo; i++)
    {
        op = rand() % 10;
        k = rand() % rango;
        v = rand();
        va = -1;
        vb = -1;

        if(op < 5)
        {
            resA = agregarLruCacheLista(&a, &k, &v, sizeof(v));
            resB = agregarLruCacheArbol(&b, &k, &v, sizeof(v));
        }
        else if(op < 9)
        {
            resA = obtenerLruCacheLista(&a, &k, &va, sizeof(va));
            resB = obtenerLruCacheArbol(&b, &k, &vb, sizeof(vb));
        }
        else
        {
            resA = borrarLruCacheLista(&a, &k);
            resB = borrarLruCacheArbol(&b, &k);
        }

        if(resA != resB || va != vb || !consistentes(&a, &b))
            fallo = i;
    }

    vaciarLruCacheLista(&a);
    vaciarLruCacheArbol(&b);

    if(!fallo && (a.cantidad != 0 || b.cantidad != 0 || a.lista || b.indice || b.lista.ini || b.lista.fin))
        fallo = operaciones + 1;           /* vaciar dejo restos */

    *fuga = BALANCE_MEM() - antes;         /* medir antes de imprimir */

    return fallo;
}

/* Prueba varias capacidades y rangos de claves; informa fallos y fugas de memoria */
int ejecutarTestCruzado(void)
{
    unsigned capacidades[] = {1, 2, 5, 50, 200};
    int factores[] = {1, 3};               /* rango de claves = capacidad * factor */
    unsigned *pc;
    int *pf;
    unsigned *finC = capacidades + sizeof(capacidades) / sizeof(*capacidades);
    int *finF = factores + sizeof(factores) / sizeof(*factores);
    int fallo;
    int errores = 0;
    long fuga;

    printf("%-10s %-8s %-10s %s\n", "capacidad", "rango", "resultado", "fugas");

    for(pc = capacidades; pc < finC; pc++)
    {
        for(pf = factores; pf < finF; pf++)
        {
            fallo = ejecutarEscenario(*pc, (int)*pc * *pf, 1234, 20000, &fuga);

            printf("%-10u %-8d ", *pc, (int)*pc * *pf);

            if(fallo)
                printf("FALLO en operacion %d", fallo);
            else
                printf("OK        ");

#ifdef CONTAR_MEM
            printf(" %ld\n", fuga);
            errores += (fuga != 0);
#else
            printf(" (no medido)\n");
#endif
            errores += (fallo != 0);
        }
    }

    printf("\n%s\n", errores ? "HAY ERRORES" : "TODO OK");

    return errores != 0;
}
