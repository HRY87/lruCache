#ifndef LRUCACHEARBOL_H_INCLUDED
#define LRUCACHEARBOL_H_INCLUDED

#include "listaDoble.h"
#include "arbol.h"

/* La lista guarda [clave][dato] y es duena de la memoria del dato.
   El arbol indice guarda [clave][puntero al nodo de la lista]: tiene su
   propia copia de la clave y nunca libera el nodo de la lista. */
typedef struct
{
    tListaD lista;         /* Cache -> ini = usado mas recientemente, fin = menos */
    tArbol indice;
    unsigned cantidad;
    unsigned capacidad;
    unsigned tamClave;
    tCmp cmp;
} tLruCacheArbol;

int crearLruCacheArbol(tLruCacheArbol *c, unsigned capacidad, unsigned tamClave, tCmp cmp);
int agregarLruCacheArbol(tLruCacheArbol *c, const void *clave, const void *dato, unsigned tamDato);
int obtenerLruCacheArbol(tLruCacheArbol *c, const void *clave, void *dato, unsigned tamDato);
int borrarLruCacheArbol(tLruCacheArbol *c, const void *clave);
void vaciarLruCacheArbol(tLruCacheArbol *c);
unsigned alturaLruCacheArbol(const tLruCacheArbol *c);

#endif // LRUCACHEARBOL_H_INCLUDED
