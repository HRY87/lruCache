#ifndef LISTA_H_INCLUDED
#define LISTA_H_INCLUDED

#include "utilidades.h"

typedef struct sNodo
{
    void            *info;
    unsigned        tamInfo;
    struct sNodo    *sig;
} tNodo;

typedef tNodo *tLista;

void crearLista(tLista *pl);
void vaciarLista(tLista *pl);
int ponerAlComienzoLista(tLista *pl, const void *dato, unsigned cantBytes);
int sacarUltimoLista(tLista *pl, void *dato, unsigned cantBytes);

/* Adicionales: trabajan con el enlace (tLista*) que apunta al nodo */
tLista *buscarClaveLista(tLista *pl, const void *clave, tCmp cmp);
void moverAlComienzoLista(tLista *pl, tLista *pos);
void eliminarNodoLista(tLista *pos);

#endif // LISTA_H_INCLUDED
