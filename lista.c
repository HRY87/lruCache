#include "lista.h"

/* Deja la lista vacia */
void crearLista(tLista *pl)
{
    *pl = NULL;
}

/* Libera todos los nodos y sus datos */
void vaciarLista(tLista *pl)
{
    tNodo *elim;

    while(*pl)
    {
        elim = *pl;
        *pl = elim->sig;
        free(elim->info);
        free(elim);
    }
}

/* Copia el dato en un nodo nuevo al comienzo de la lista */
int ponerAlComienzoLista(tLista *pl, const void *dato, unsigned cantBytes)
{
    tNodo *nue = malloc(sizeof(tNodo));

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
    nue->sig = *pl;
    *pl = nue;

    return TODO_OK;
}

/* Saca el ultimo nodo; si dato no es NULL, copia en el hasta cantBytes de su info */
int sacarUltimoLista(tLista *pl, void *dato, unsigned cantBytes)
{
    tNodo *elim;

    if(!*pl)
        return LISTA_VACIA;

    while((*pl)->sig)
        pl = &(*pl)->sig;

    elim = *pl;

    if(dato)
        memcpy(dato, elim->info, MINIMO(cantBytes, elim->tamInfo));

    *pl = NULL;
    free(elim->info);
    free(elim);

    return TODO_OK;
}

/* Devuelve el enlace al nodo con esa clave, o el NULL final si no existe */
tLista *buscarClaveLista(tLista *pl, const void *clave, tCmp cmp)
{
    while(*pl && cmp(clave, (*pl)->info) != 0)
        pl = &(*pl)->sig;

    return pl;
}

/* Mueve al comienzo el nodo apuntado por pos, sin crear ni liberar memoria */
void moverAlComienzoLista(tLista *pl, tLista *pos)
{
    tNodo *nodo = *pos;

    if(pos != pl)
    {
        *pos = nodo->sig;
        nodo->sig = *pl;
        *pl = nodo;
    }
}

/* Quita y libera el nodo apuntado por pos */
void eliminarNodoLista(tLista *pos)
{
    tNodo *elim = *pos;

    *pos = elim->sig;
    free(elim->info);
    free(elim);
}
