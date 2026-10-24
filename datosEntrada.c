#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "datosEntrada.h"

/* Descarta lo que quede de la linea actual de la entrada */
static void descartarLinea(void)
{
    int c = getchar();

    while(c != '\n' && c != EOF)
        c = getchar();
}

/* Lee un numero; si la linea esta vacia (o no hay entrada) devuelve defecto */
unsigned long leerNumero(const char *mensaje, unsigned long defecto)
{
    char buf[32];
    char *fin;
    unsigned long n;

    printf("%s [%lu]: ", mensaje, defecto);

    if(!fgets(buf, sizeof(buf), stdin))
        return defecto;

    n = strtoul(buf, &fin, 10);

    return fin == buf ? defecto : n;
}

/* Lee un nombre valido (letras, numeros o '_'); linea vacia = defecto */
int leerNombre(const char *mensaje, const char *defecto, char *dest, size_t tam)
{
    char buf[LARGO_NOMBRE + 2];
    size_t largo;
    const char *p;
    int ok;

    printf("%s [%s]: ", mensaje, defecto);

    if(!fgets(buf, sizeof(buf), stdin))
        buf[0] = '\0';

    largo = strcspn(buf, "\n");

    if(buf[largo] == '\0' && largo == sizeof(buf) - 1)      /* linea demasiado larga */
        descartarLinea();

    buf[largo] = '\0';

    if(largo == 0)
    {
        snprintf(buf, sizeof(buf), "%s", defecto);
        largo = strlen(buf);
    }

    ok = largo < tam;

    for(p = buf; *p && ok; p++)
        ok = isalnum((unsigned char)*p) || *p == '_';

    if(!ok)
        printf("Nombre invalido (hasta %u letras, numeros o _).\n", (unsigned)(tam - 1));
    else
        memcpy(dest, buf, largo + 1);

    return ok;
}
