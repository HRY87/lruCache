#include "listaDoble.h"

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

    nue->ant = NULL;
    nue->sig = pl->ini;

    if(pl->ini)
        pl->ini->ant = nue;
    else
        pl->fin = nue;

    pl->ini = nue;

    return TODO_OK;
}

/* Lleva un nodo de la lista al inicio en tiempo constante, sin crear ni liberar memoria */
void moverAlInicioListaDoble(tListaD *pl, tNodoD *nodo)
{
    if(nodo != pl->ini)         /* si no es el primero, tiene anterior */
    {
        nodo->ant->sig = nodo->sig;

        if(nodo->sig)
            nodo->sig->ant = nodo->ant;
        else
            pl->fin = nodo->ant;

        nodo->ant = NULL;
        nodo->sig = pl->ini;
        pl->ini->ant = nodo;
        pl->ini = nodo;
    }
}

/* Quita y libera un nodo de la lista en tiempo constante */
void eliminarNodoListaDoble(tListaD *pl, tNodoD *nodo)
{
    if(nodo->ant)
        nodo->ant->sig = nodo->sig;
    else
        pl->ini = nodo->sig;

    if(nodo->sig)
        nodo->sig->ant = nodo->ant;
    else
        pl->fin = nodo->ant;

    free(nodo->info);
    free(nodo);
}
