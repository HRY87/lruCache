#include "lruCacheLista.h"

/* Inicializa la cache con su capacidad maxima, tamanio de clave y comparador */
int crearLruCacheLista(tLruCacheLista *c, unsigned capacidad, unsigned tamClave, tCmp cmp)
{
    if(!c || !cmp || !capacidad || !tamClave)
        return PARAM_INVALIDO;

    crearLista(&c->lista);
    c->cantidad = 0;
    c->capacidad = capacidad;
    c->tamClave = tamClave;
    c->cmp = cmp;

    return TODO_OK;
}

/* Agrega al inicio; si la clave existia la reemplaza; si se desborda, descarta el ultimo */
int agregarLruCacheLista(tLruCacheLista *c, const void *clave, const void *dato, unsigned tamDato)
{
    tLista *pos = buscarClaveLista(&c->lista, clave, c->cmp);
    void *entrada;
    int res;

    if(*pos)
    {
        eliminarNodoLista(pos);
        c->cantidad--;
    }

    entrada = crearEntrada(clave, c->tamClave, dato, tamDato);

    if(!entrada)
        return SIN_MEM;

    res = ponerAlComienzoLista(&c->lista, entrada, c->tamClave + tamDato);
    free(entrada);

    if(res != TODO_OK)
        return res;

    c->cantidad++;

    if(c->cantidad > c->capacidad)
    {
        sacarUltimoLista(&c->lista, NULL, 0);
        c->cantidad--;
    }

    return TODO_OK;
}

/* Busca por clave, lo mueve al inicio y copia su dato */
int obtenerLruCacheLista(tLruCacheLista *c, const void *clave, void *dato, unsigned tamDato)
{
    tLista *pos = buscarClaveLista(&c->lista, clave, c->cmp);

    if(!*pos)
        return NO_EXISTE;

    moverAlComienzoLista(&c->lista, pos);
    copiarDatoEntrada(c->lista->info, c->lista->tamInfo, c->tamClave, dato, tamDato);

    return TODO_OK;
}

/* Elimina la entrada con esa clave */
int borrarLruCacheLista(tLruCacheLista *c, const void *clave)
{
    tLista *pos = buscarClaveLista(&c->lista, clave, c->cmp);

    if(!*pos)
        return NO_EXISTE;

    eliminarNodoLista(pos);
    c->cantidad--;

    return TODO_OK;
}

/* Libera todas las entradas */
void vaciarLruCacheLista(tLruCacheLista *c)
{
    vaciarLista(&c->lista);
    c->cantidad = 0;
}
