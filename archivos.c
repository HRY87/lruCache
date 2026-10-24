#include <stdio.h>
#include "archivos.h"
#include "registros.h"
#include "utilidades.h"

/* Buffer con lugar para cualquiera de los dos registros */
typedef union
{
    tPosteo posteo;
    tUsuario usuario;
} tRegistro;

typedef void (*tMostrar)(unsigned long pos, const void *registro);

/* Muestra un posteo; avisa si su id no coincide con su posicion en el archivo */
static void mostrarPosteo(unsigned long pos, const void *registro)
{
    const tPosteo *post = registro;

    printf("[%lu] id=%d%s texto: %.*s\n", pos, post->id,
           (unsigned long)post->id == pos ? "" : " (NO COINCIDE CON LA POSICION)",
           LARGO_TEXTO, post->texto);
}

/* Muestra un usuario con los ids de su feed */
static void mostrarUsuario(unsigned long pos, const void *registro)
{
    const tUsuario *usu = registro;
    const int *p;

    printf("[%lu] %.*s:", pos, LARGO_USUARIO, usu->usuario);

    for(p = usu->feed; p < usu->feed + LARGO_FEED; p++)
        printf(" %d", *p);

    printf("\n");
}

/* Recorre 'cantidad' registros de tamReg bytes desde 'desde', sin leer el resto del archivo */
static int mostrarRegistros(const char *nombre, unsigned tamReg, unsigned long desde,
                            unsigned long cantidad, tMostrar mostrar)
{
    FILE *pf;
    tRegistro reg;
    unsigned long total, pos, fin;
    long bytes;

    if(tamReg > sizeof(reg))
        return PARAM_INVALIDO;

    pf = fopen(nombre, "rb");

    if(!pf)
    {
        printf("No se pudo abrir %s (generalo primero)\n", nombre);
        return NO_EXISTE;
    }

    fseek(pf, 0, SEEK_END);
    bytes = ftell(pf);
    total = (unsigned long)bytes / tamReg;

    printf("\n%s: %lu registros de %u bytes (%ld bytes en total)\n", nombre, total, tamReg, bytes);

    if(desde >= total)
    {
        printf("El registro %lu no existe\n", desde);
        fclose(pf);
        return PARAM_INVALIDO;
    }

    fin = cantidad > total - desde ? total : desde + cantidad;
    fseek(pf, (long)(desde * tamReg), SEEK_SET);

    for(pos = desde; pos < fin && fread(&reg, tamReg, 1, pf) == 1; pos++)
        mostrar(pos, &reg);

    fclose(pf);

    return TODO_OK;
}

/* Muestra registros de posteos.dat */
int verPosteos(unsigned long desde, unsigned long cantidad)
{
    return mostrarRegistros(NOMBRE_POSTEOS, sizeof(tPosteo), desde, cantidad, mostrarPosteo);
}

/* Muestra registros de usuarios_<lote>.dat */
int verUsuarios(const char *lote, unsigned long desde, unsigned long cantidad)
{
    char nombre[64];

    snprintf(nombre, sizeof(nombre), FORMATO_USUARIOS, lote);

    return mostrarRegistros(nombre, sizeof(tUsuario), desde, cantidad, mostrarUsuario);
}
