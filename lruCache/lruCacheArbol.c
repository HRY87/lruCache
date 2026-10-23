#include "lruCacheArbol.h"

/* Lee del nodo del indice el puntero al nodo de la lista (memcpy: puede estar desalineado) */
static tNodoD *nodoDeIndice(const tLruCacheArbol *c, const tNodoA *nodoIndice)
{
    tNodoD *nodo;

    memcpy(&nodo, (const char *)nodoIndice->dato + c->tamClave, sizeof(nodo));

    return nodo;
}

/* Quita una entrada de las DOS estructuras. pos es su enlace en el indice. */
static void quitarEntrada(tLruCacheArbol *c, tArbol *pos)
{
    tNodoD *nodo = nodoDeIndice(c, *pos);   /* leerlo antes de tocar el arbol */

    eliminarEnlaceArbol(pos);
    eliminarNodoListaDoble(&c->lista, nodo);
    c->cantidad--;
}

/* Descarta la entrada menos usada. La clave se toma del nodo de la lista,
   que todavia existe cuando se busca en el arbol. */
static void eliminarUltimo(tLruCacheArbol *c)
{
    quitarEntrada(c, buscarEnlaceArbol(&c->indice, c->lista.fin->info, c->cmp));
}

/* Crea la entrada en la lista y en el indice (en el enlace pos, ya buscado) */
static int insertarEntrada(tLruCacheArbol *c, tArbol *pos, const void *clave, const void *dato, unsigned tamDato)
{
    tNodoD *nodo;
    void *entrada = crearEntrada(clave, c->tamClave, dato, tamDato);
    int res;

    if(!entrada)
        return SIN_MEM;

    res = ponerAlInicioListaDoble(&c->lista, entrada, c->tamClave + tamDato);
    free(entrada);

    if(res != TODO_OK)
        return res;

    nodo = c->lista.ini;
    entrada = crearEntrada(clave, c->tamClave, &nodo, sizeof(nodo));
    res = entrada ? insertarEnEnlaceArbol(pos, entrada, c->tamClave + sizeof(nodo)) : SIN_MEM;
    free(entrada);

    if(res != TODO_OK)
    {
        eliminarNodoListaDoble(&c->lista, nodo);   /* deshace el alta en la lista */
        return res;
    }

    c->cantidad++;

    if(c->cantidad > c->capacidad)
        eliminarUltimo(c);

    return TODO_OK;
}

/* Inicializa la cache con su capacidad maxima, tamanio de clave y comparador */
int crearLruCacheArbol(tLruCacheArbol *c, unsigned capacidad, unsigned tamClave, tCmp cmp)
{
    if(!c || !cmp || !capacidad || !tamClave)
        return PARAM_INVALIDO;

    crearListaDoble(&c->lista);
    crearArbol(&c->indice);
    c->cantidad = 0;
    c->capacidad = capacidad;
    c->tamClave = tamClave;
    c->cmp = cmp;

    return TODO_OK;
}

/* Agrega al inicio; si la clave existia la reemplaza; si se desborda, descarta el ultimo */
int agregarLruCacheArbol(tLruCacheArbol *c, const void *clave, const void *dato, unsigned tamDato)
{
    tArbol *pos = buscarEnlaceArbol(&c->indice, clave, c->cmp);

    if(*pos)
    {
        quitarEntrada(c, pos);
        pos = buscarEnlaceArbol(&c->indice, clave, c->cmp);   /* el arbol cambio */
    }

    return insertarEntrada(c, pos, clave, dato, tamDato);
}

/* Busca en el indice, mueve la entrada al inicio de la lista y copia su dato */
int obtenerLruCacheArbol(tLruCacheArbol *c, const void *clave, void *dato, unsigned tamDato)
{
    tArbol *pos = buscarEnlaceArbol(&c->indice, clave, c->cmp);
    tNodoD *nodo;

    if(!*pos)
        return NO_EXISTE;

    nodo = nodoDeIndice(c, *pos);
    moverAlInicioListaDoble(&c->lista, nodo);
    copiarDatoEntrada(nodo->info, nodo->tamInfo, c->tamClave, dato, tamDato);

    return TODO_OK;
}

/* Elimina la entrada con esa clave de las dos estructuras */
int borrarLruCacheArbol(tLruCacheArbol *c, const void *clave)
{
    tArbol *pos = buscarEnlaceArbol(&c->indice, clave, c->cmp);

    if(!*pos)
        return NO_EXISTE;

    quitarEntrada(c, pos);

    return TODO_OK;
}

/* Libera el indice y la lista (cada una libera solo lo suyo) */
void vaciarLruCacheArbol(tLruCacheArbol *c)
{
    vaciarArbol(&c->indice);
    vaciarListaDoble(&c->lista);
    c->cantidad = 0;
}

/* Altura del arbol indice */
unsigned alturaLruCacheArbol(const tLruCacheArbol *c)
{
    return alturaArbol(&c->indice);
}
