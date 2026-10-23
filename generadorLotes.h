#ifndef GENERADORLOTES_H_INCLUDED
#define GENERADORLOTES_H_INCLUDED

#define USUARIOS_MIN     200
#define USUARIOS_MAX     99999
#define POOL_DEFECTO     100

/* tipo: "concentrado", "disperso" u "ordenado". Devuelve TODO_OK o un codigo de error. */
int generarLote(unsigned long semilla, const char *tipo, int usuarios, int tamPool);

#endif // GENERADORLOTES_H_INCLUDED
