#include "exportador.h"
#include "registros.h"

/* Arma en txt el nombre de bin cambiando la extension por .txt; 0 si no entra */
static int armarNombreTexto(const char *bin, char *txt, size_t tam)
{
    const char *punto = strrchr(bin, '.');
    size_t largo = punto ? (size_t)(punto - bin) : strlen(bin);

    if(largo + 5 > tam)                 /* ".txt" y el terminador */
        return 0;

    memcpy(txt, bin, largo);
    strcpy(txt + largo, ".txt");

    return 1;
}

/* Lee el binario registro a registro y escribe una linea de texto por cada uno */
int exportarATexto(const char *nombreBin, unsigned tamRegistro, tFormatoRegistro formatear)
{
    char nombreTxt[64];
    FILE *pb = NULL;
    FILE *pt = NULL;
    void *reg = malloc(tamRegistro);
    long numero = 0;
    int ok = reg && armarNombreTexto(nombreBin, nombreTxt, sizeof(nombreTxt));

    if(ok)
    {
        pb = fopen(nombreBin, "rb");
        pt = fopen(nombreTxt, "w");
        ok = pb && pt;
    }

    while(ok && fread(reg, tamRegistro, 1, pb) == 1)
    {
        formatear(pt, reg, numero);
        numero++;
    }

    fclose(pb);
    fclose(pt);
    free(reg);

    return ok ? TODO_OK : PARAM_INVALIDO;
}

/* Una linea por posteo: id y texto */
void formatearPosteo(FILE *pt, const void *registro, long numero)
{
    const tPosteo *post = registro;

    fprintf(pt, "%6ld | id %6d | %.*s\n", numero, post->id, LARGO_TEXTO, post->texto);
}

/* Una linea por usuario: nombre y los ids de su feed */
void formatearUsuario(FILE *pt, const void *registro, long numero)
{
    const tUsuario *usu = registro;
    const int *p;

    fprintf(pt, "%6ld | %.*s |", numero, LARGO_USUARIO, usu->usuario);

    for(p = usu->feed; p < usu->feed + LARGO_FEED; p++)
        fprintf(pt, " %d", *p);

    fputc('\n', pt);
}
