#ifndef DATOSENTRADA_H_INCLUDED
#define DATOSENTRADA_H_INCLUDED

#include <stddef.h>

#define LARGO_NOMBRE    32      /* incluye el '\0' */

/* Lee un numero; si la linea esta vacia (o no hay entrada) devuelve defecto */
unsigned long leerNumero(const char *mensaje, unsigned long defecto);

/* Lee un nombre de letras, numeros o '_' (largo menor a tam). Si la linea esta
   vacia usa defecto. Devuelve 1 si es valido, 0 si no (y avisa). */
int leerNombre(const char *mensaje, const char *defecto, char *dest, size_t tam);

#endif // DATOSENTRADA_H_INCLUDED
