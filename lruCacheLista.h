#ifndef LRUCACHELISTA_H_INCLUDED
#define LRUCACHELISTA_H_INCLUDED

#include "lista.h"

typedef struct
{
    tLista lista;          /*Cache -> inicio = usado mas recientemente */
    unsigned cantidad;
    unsigned capacidad;
    unsigned tamClave;     /* Todas las claves de una cache tienen el mismo tipo */
    tCmp cmp;              /* Se pasa una sola vez, por lo tanto el comparador debe ser propio de la cache*/
} tLruCacheLista;

int crearLruCacheLista(tLruCacheLista *c, unsigned capacidad, unsigned tamClave, tCmp cmp);
int agregarLruCacheLista(tLruCacheLista *c, const void *clave, const void *dato, unsigned tamDato);
int obtenerLruCacheLista(tLruCacheLista *c, const void *clave, void *dato, unsigned tamDato);
int borrarLruCacheLista(tLruCacheLista *c, const void *clave);
void vaciarLruCacheLista(tLruCacheLista *c);

#endif // LRUCACHELISTA_H_INCLUDED
