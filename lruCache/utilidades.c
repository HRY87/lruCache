#include "utilidades.h"

/* Compara dos claves enteras sin riesgo de desbordamiento */
int cmpInt(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;

    return (x > y) - (x < y);
}

/* Arma el bloque [clave][dato]. Devuelve NULL si no hay memoria. */
void *crearEntrada(const void *clave, unsigned tamClave, const void *dato, unsigned tamDato)
{
    char *entrada = malloc(tamClave + tamDato);

    if(entrada)
    {
        memcpy(entrada, clave, tamClave);
        memcpy(entrada + tamClave, dato, tamDato);
    }

    return entrada;
}

/* Copia a 'dato' la parte del dato de una entrada (hasta tamDato bytes) */
void copiarDatoEntrada(const void *entrada, unsigned tamEntrada, unsigned tamClave, void *dato, unsigned tamDato)
{
    memcpy(dato, (const char *)entrada + tamClave, MINIMO(tamDato, tamEntrada - tamClave));
}
