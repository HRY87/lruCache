#ifndef LISTADOBLE_H_INCLUDED
#define LISTADOBLE_H_INCLUDED

#include "utilidades.h"

typedef struct sNodoD
{
    void            *info;
    unsigned        tamInfo;
    struct sNodoD   *sig,
                    *ant;
} tNodoD;

/* Guarda los dos extremos para no recorrer la lista nunca */
typedef struct
{
    tNodoD *ini;
    tNodoD *fin;
} tListaD;

void crearListaDoble(tListaD *pl);
int vaciarListaDoble(tListaD *pl);
int ponerAlInicioListaDoble(tListaD *pl, const void *dato, unsigned cantBytes);
void moverAlInicioListaDoble(tListaD *pl, tNodoD *nodo);
void eliminarNodoListaDoble(tListaD *pl, tNodoD *nodo);

#endif // LISTADOBLE_H_INCLUDED
