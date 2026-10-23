#ifndef UTILIDADES_H_INCLUDED
#define UTILIDADES_H_INCLUDED

#include <stdlib.h>
#include <string.h>

#define MINIMO(X,Y)     ((X)<(Y)?(X):(Y))

#define TODO_OK          0
#define SIN_MEM          1
#define NO_EXISTE        2
#define LISTA_VACIA      3
#define PARAM_INVALIDO   4

/* Compara dos claves: negativo, cero o positivo segun a sea menor, igual o mayor que b */
typedef int (*tCmp)(const void *a, const void *b);

int cmpInt(const void *a, const void *b);

/* Una entrada es un unico bloque [clave][dato]: la clave queda al inicio,
   asi tCmp puede comparar directamente contra el bloque guardado. */
void *crearEntrada(const void *clave, unsigned tamClave, const void *dato, unsigned tamDato);
void copiarDatoEntrada(const void *entrada, unsigned tamEntrada, unsigned tamClave, void *dato, unsigned tamDato);

#endif // UTILIDADES_H_INCLUDED
