#ifndef ARCHIVOS_H_INCLUDED
#define ARCHIVOS_H_INCLUDED

/* Muestran 'cantidad' registros a partir del registro 'desde' (el primero es el 0).
   Devuelven TODO_OK o un codigo de error. */
int verPosteos(unsigned long desde, unsigned long cantidad);
int verUsuarios(const char *lote, unsigned long desde, unsigned long cantidad);


#endif // ARCHIVOS_H_INCLUDED
