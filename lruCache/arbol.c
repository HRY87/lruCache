#include "arbol.h"

/* Intercambia los datos de dos nodos (no los nodos) */
static void intercambiarDatos(tNodoA *a, tNodoA *b)
{
    void *auxDato = a->dato;
    unsigned auxTam = a->tam;

    a->dato = b->dato;
    a->tam = b->tam;
    b->dato = auxDato;
    b->tam = auxTam;
}

/* Deja el arbol vacio */
void crearArbol(tArbol *pa)
{
    *pa = NULL;
}

/* Libera nodos y datos sin recursion (no desborda la pila aunque el arbol
   este degenerado): rota a la derecha hasta que la raiz no tiene hijo
   izquierdo y entonces la libera. */
void vaciarArbol(tArbol *pa)
{
    tNodoA *nodo;
    tNodoA *izq;

    while(*pa)
    {
        nodo = *pa;

        if(nodo->izq)
        {
            izq = nodo->izq;
            nodo->izq = izq->der;
            izq->der = nodo;
            *pa = izq;
        }
        else
        {
            *pa = nodo->der;
            free(nodo->dato);
            free(nodo);
        }
    }
}

/* Cantidad de niveles del arbol */
unsigned alturaArbol(const tArbol *pa)
{
    unsigned izq;
    unsigned der;

    if(!*pa)
        return 0;

    izq = alturaArbol(&(*pa)->izq);
    der = alturaArbol(&(*pa)->der);

    return 1 + (izq > der ? izq : der);
}

/* Devuelve el enlace al nodo con esa clave, o el enlace NULL donde se
   insertaria si no existe. La clave se compara contra el inicio de dato. */
tArbol *buscarEnlaceArbol(tArbol *pa, const void *clave, tCmp cmp)
{
    int comp = 0;

    while(*pa && (comp = cmp(clave, (*pa)->dato)) != 0)
        pa = comp < 0 ? &(*pa)->izq : &(*pa)->der;

    return pa;
}

/* Crea un nodo con una copia del dato en el enlace pos (que debe ser NULL) */
int insertarEnEnlaceArbol(tArbol *pos, const void *dato, unsigned tam)
{
    tNodoA *nue = malloc(sizeof(tNodoA));

    if(!nue)
        return SIN_MEM;

    nue->dato = malloc(tam);

    if(!nue->dato)
    {
        free(nue);
        return SIN_MEM;
    }

    memcpy(nue->dato, dato, tam);
    nue->tam = tam;
    nue->izq = NULL;
    nue->der = NULL;
    *pos = nue;

    return TODO_OK;
}

/* Elimina el nodo apuntado por pos. Con dos hijos, intercambia el dato con
   el sucesor inorden y elimina ese nodo, que tiene a lo sumo un hijo. */
void eliminarEnlaceArbol(tArbol *pos)
{
    tNodoA *elim = *pos;
    tArbol *suc;

    if(elim->izq && elim->der)
    {
        suc = &elim->der;

        while((*suc)->izq)
            suc = &(*suc)->izq;

        intercambiarDatos(elim, *suc);
        pos = suc;
        elim = *pos;
    }

    *pos = elim->izq ? elim->izq : elim->der;
    free(elim->dato);
    free(elim);
}
