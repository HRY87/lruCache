#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "loteManual.h"
#include "generadorLotes.h"
#include "registros.h"
#include "datosEntrada.h"
#include "utilidades.h"

#define TAM_LINEA       512

#define LINEA_OK        0
#define LINEA_INVALIDA  1
#define LINEA_FIN       2       /* se cerro la entrada */

/* Indica si el archivo ya existe */
static int existeArchivo(const char *nombre)
{
    FILE *pf = fopen(nombre, "rb");
    int existe = pf != NULL;

    if(pf)
        fclose(pf);

    return existe;
}

/* Lee una linea con 1 a LARGO_FEED ids (0 a CANT_POSTEOS-1) separados por espacios.
   Deja en *cant cuantos leyo. */
static int leerIds(int *ids, int *cant)
{
    char linea[TAM_LINEA];
    char *p = linea;
    char *fin;
    long id;

    if(!fgets(linea, sizeof(linea), stdin))
        return LINEA_FIN;

    *cant = 0;
    id = strtol(p, &fin, 10);

    while(fin != p && id >= 0 && id < CANT_POSTEOS && *cant < LARGO_FEED)
    {
        *(ids + (*cant)++) = (int)id;
        p = fin;
        id = strtol(p, &fin, 10);
    }

    while(isspace((unsigned char)*p))
        p++;

    /* valida si se consumio toda la linea y hay al menos un id */
    return (*p == '\0' && *cant > 0) ? LINEA_OK : LINEA_INVALIDA;
}

/* Completa el feed repitiendo desde el principio los ids ingresados */
static void completarFeed(int *feed, int cant)
{
    int *p = feed + cant;

    while(p < feed + LARGO_FEED)
    {
        *p = *(p - cant);
        p++;
    }
}

/* Arma el usuario numero 'numero' pidiendole los ids a quien usa el programa.
   Devuelve 0 si se cerro la entrada. */
static int pedirUsuario(tUsuario *usu, int numero)
{
    int cant = 0;
    int res;

    memset(usu, 0, sizeof(*usu));
    snprintf(usu->usuario, sizeof(usu->usuario), FORMATO_NOMBRE_USUARIO, (unsigned)numero % 100000u);

    do
    {
        printf("%s - ids: ", usu->usuario);
        res = leerIds(usu->feed, &cant);

        if(res == LINEA_INVALIDA)
            printf("  Entrada invalida: de 1 a %d ids enteros entre 0 y %d\n", LARGO_FEED, CANT_POSTEOS - 1);
    }
    while(res == LINEA_INVALIDA);

    if(res == LINEA_OK)
        completarFeed(usu->feed, cant);

    return res == LINEA_OK;
}

/* Crea el lote manual: pide cantidad de usuarios y el feed de cada uno */
int crearLoteManual(const char *lote)
{
    char archivo[64];
    FILE *pf;
    tUsuario usu;
    int usuarios;
    int i;
    int ok = 1;

    snprintf(archivo, sizeof(archivo), FORMATO_USUARIOS, lote);

    if(existeArchivo(archivo) && leerNumero("Ya existe, sobrescribir (1 si, 0 no)", 0) != 1)
    {
        printf("No se creo el lote.\n");
        return PARAM_INVALIDO;
    }

    usuarios = (int)leerNumero("Cantidad de usuarios", 1);

    if(usuarios < 1 || usuarios > USUARIOS_MAX)
    {
        printf("Cantidad invalida (1 a %d)\n", USUARIOS_MAX);
        return PARAM_INVALIDO;
    }

    pf = fopen(archivo, "wb");

    if(!pf)
    {
        printf("No se pudo crear %s\n", archivo);
        return PARAM_INVALIDO;
    }

    printf("Por cada usuario escribi de 1 a %d ids de posteos (0 a %d) separados por espacios.\n"
           "Si escribis menos de %d, la lista se repite hasta completarlos.\n",
           LARGO_FEED, CANT_POSTEOS - 1, LARGO_FEED);

    for(i = 1; i <= usuarios && ok; i++)
        ok = pedirUsuario(&usu, i) && fwrite(&usu, sizeof(usu), 1, pf) == 1;

    fclose(pf);

    if(!ok)
    {
        remove(archivo);
        printf("Lote cancelado: no se guardo %s\n", archivo);
        return PARAM_INVALIDO;
    }

    printf("Lote '%s': %d usuarios en %s\n", lote, usuarios, archivo);

    return TODO_OK;
}
