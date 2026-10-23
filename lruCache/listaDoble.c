#include "listaDoble.h"

/* Saca el nodo de la lista sin liberarlo */
static void desenlazar(tListaD *pl, tNodoD *nodo)
{
    if(nodo->ant)
        nodo->ant->sig = nodo->sig;
    else
        pl->ini = nodo->sig;

    if(nodo->sig)
        nodo->sig->ant = nodo->ant;
    else
        pl->fin = nodo->ant;
}

/* Engancha un nodo ya existente al inicio */
static void enlazarAlInicio(tListaD *pl, tNodoD *nodo)
{
    nodo->ant = NULL;
    nodo->sig = pl->ini;

    if(pl->ini)
        pl->ini->ant = nodo;
    else
        pl->fin = nodo;

    pl->ini = nodo;
}

/* Deja la lista vacia */
void crearListaDoble(tListaD *pl)
{
    pl->ini = NULL;
    pl->fin = NULL;
}

/* Libera todos los nodos y sus datos; devuelve cuantos libero */
int vaciarListaDoble(tListaD *pl)
{
    int cant = 0;
    tNodoD *elim;

    while(pl->ini)
    {
        elim = pl->ini;
        pl->ini = elim->sig;
        free(elim->info);
        free(elim);
        cant++;
    }

    pl->fin = NULL;

    return cant;
}

/* Copia el dato en un nodo nuevo al inicio (queda en pl->ini) */
int ponerAlInicioListaDoble(tListaD *pl, const void *dato, unsigned cantBytes)
{
    tNodoD *nue = malloc(sizeof(tNodoD));

    if(!nue)
        return SIN_MEM;

    nue->info = malloc(cantBytes);

    if(!nue->info)
    {
        free(nue);
        return SIN_MEM;
    }

    memcpy(nue->info, dato, cantBytes);
    nue->tamInfo = cantBytes;
    enlazarAlInicio(pl, nue);

    return TODO_OK;
}

/* Lleva un nodo de la lista al inicio en tiempo constante */
void moverAlInicioListaDoble(tListaD *pl, tNodoD *nodo)
{
    if(nodo != pl->ini)
    {
        desenlazar(pl, nodo);
        enlazarAlInicio(pl, nodo);
    }
}

/* Quita y libera un nodo de la lista en tiempo constante */
void eliminarNodoListaDoble(tListaD *pl, tNodoD *nodo)
{
    desenlazar(pl, nodo);
    free(nodo->info);
    free(nodo);
}
