#ifndef ARBOL_H_INCLUDED
#define ARBOL_H_INCLUDED

#include "utilidades.h"

typedef struct sNodoA
{
    void            *dato;
    unsigned        tam;
    struct sNodoA   *izq;
    struct sNodoA   *der;
} tNodoA;

typedef tNodoA *tArbol;

void crearArbol(tArbol *pa);
void vaciarArbol(tArbol *pa);
unsigned alturaArbol(const tArbol *pa);

/* Trabajan con el enlace (tArbol*) que apunta al nodo */
tArbol *buscarEnlaceArbol(tArbol *pa, const void *clave, tCmp cmp);
int insertarEnEnlaceArbol(tArbol *pos, const void *dato, unsigned tam);
void eliminarEnlaceArbol(tArbol *pos);

#endif // ARBOL_H_INCLUDED
