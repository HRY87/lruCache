#ifndef GENERADORLOTES_H_INCLUDED
#define GENERADORLOTES_H_INCLUDED

#include <stdio.h>

#define USUARIOS_MIN     200
#define USUARIOS_MAX     99999
#define POOL_DEFECTO     100

#define MAX_CARACTERES   139            /* "menos de 140" */

/* Elegir aca como se arman los textos de los posteos */
#define TEXTO_LETRAS     0
#define TEXTO_PALABRAS   1
#define TEXTO_ELEGIDO    TEXTO_PALABRAS

typedef struct
{
    int *pool;                          /* posteos populares (lote concentrado) */
    int tamPool;
    unsigned long contador;             /* ids ya generados (lote ordenado) */
    unsigned long total;                /* ids totales a generar */
} tCtxLote;

typedef int (*tGenerarId)(tCtxLote *ctx);
typedef void (*tGenerarTexto)(char *texto);

/* Escribe un registro como una linea en pt. Devuelve distinto de 0 si pudo escribir. */
typedef int (*tFormatear)(FILE *pt, const void *registro);

typedef struct
{
    const char *nombre;
    tGenerarId generar;
} tTipoLote;

/* tipo: "concentrado", "disperso" u "ordenado". Devuelve TODO_OK o un codigo de error. */
int generarLote(unsigned long semilla, const char *tipo, int usuarios, int tamPool);

#endif // GENERADORLOTES_H_INCLUDED
