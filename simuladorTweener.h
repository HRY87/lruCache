#ifndef SIMULADORTWEENER_H_INCLUDED
#define SIMULADORTWEENER_H_INCLUDED

#include <stdio.h>
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

/* Simula el lote 'tipo' (concentrado, disperso u ordenado) sin cache, con la
   cache de la Parte 1 y con la de la Parte 2, para varias capacidades.
   Cada simulacion se repite 'repeticiones' veces y se informa el promedio. */
int ejecutarSimulacion(const char *tipo, int repeticiones);

#endif // SIMULADORTWEENER_H_INCLUDED
