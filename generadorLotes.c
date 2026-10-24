#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generadorLotes.h"
#include "registros.h"
#include "utilidades.h"
#include "exportador.h"

#define MAX_CARACTERES   139            /* "menos de 140" */
#define ALFABETO         "abcdefghijklmnopqrstuvwxyz "

typedef struct
{
    int *pool;                          /* posteos populares (lote concentrado) */
    int tamPool;
    unsigned long contador;             /* ids ya generados (lote ordenado) */
    unsigned long total;                /* ids totales a generar */
} tCtxLote;

typedef int (*tGenerarId)(tCtxLote *ctx);

typedef struct
{
    const char *nombre;
    tGenerarId generar;
} tTipoLote;

/* Numero aleatorio en [0, n). Dos rand() en sentencias separadas: orden definido y rango > RAND_MAX */
static unsigned long aleatorio(unsigned long n)
{
    unsigned long alto = (unsigned long)rand();
    unsigned long bajo = (unsigned long)rand();

    return (alto * 32768UL + bajo) % n;
}

/* Lote concentrado: elige siempre dentro de un conjunto chico de posteos populares */
static int idConcentrado(tCtxLote *ctx)
{
    return *(ctx->pool + aleatorio((unsigned long)ctx->tamPool));
}

/* Lote disperso: cualquier posteo con igual probabilidad */
static int idDisperso(tCtxLote *ctx)
{
    (void)ctx;
    return (int)aleatorio(CANT_POSTEOS);
}

/* Lote ordenado: estrictamente creciente en toda la simulacion (requiere total <= CANT_POSTEOS) */
static int idOrdenado(tCtxLote *ctx)
{
    int id = (int)((ctx->contador * CANT_POSTEOS) / ctx->total);

    ctx->contador++;

    return id;
}

/* Busca la funcion generadora de un tipo de lote; NULL si no existe */
static tGenerarId buscarTipo(const char *nombre)
{
    static const tTipoLote tipos[] =
    {
        {"concentrado", idConcentrado},
        {"disperso",    idDisperso},
        {"ordenado",    idOrdenado}
    };
    const tTipoLote *p = tipos;
    const tTipoLote *fin = tipos + sizeof(tipos) / sizeof(*tipos);

    while(p < fin && strcmp(p->nombre, nombre) != 0)
        p++;

    return p < fin ? p->generar : NULL;
}

/* Llena el texto con entre 1 y MAX_CARACTERES letras; el resto queda en 0 */
static void generarTexto(char *texto)
{
    const char *fin = texto + 1 + aleatorio(MAX_CARACTERES);
    char *p;

    for(p = texto; p < fin; p++)
        *p = *(ALFABETO + aleatorio(strlen(ALFABETO)));
}

/* Escribe el archivo de posteos: el id coincide con la posicion del registro */
static int generarPosteos(const char *nombre)
{
    FILE *pf = fopen(nombre, "wb");
    tPosteo post;
    int id;
    int ok = 1;

    if(!pf)
        return 0;

    for(id = 0; id < CANT_POSTEOS && ok; id++)
    {
        memset(&post, 0, sizeof(post));      /* archivo reproducible byte a byte */
        post.id = id;
        generarTexto(post.texto);
        ok = fwrite(&post, sizeof(post), 1, pf) == 1;
    }

    fclose(pf);

    return ok;
}

/* Escribe el archivo de usuarios, cada feed con ids del generador del lote */
static int generarUsuarios(const char *nombre, int usuarios, tGenerarId generar, tCtxLote *ctx)
{
    FILE *pf = fopen(nombre, "wb");
    tUsuario usu;
    int i;
    int *p;
    int ok = 1;

    if(!pf)
        return 0;

    for(i = 1; i <= usuarios && ok; i++)
    {
        memset(&usu, 0, sizeof(usu));
        snprintf(usu.usuario, sizeof(usu.usuario), "usuario%05d", i % 100000);

        for(p = usu.feed; p < usu.feed + LARGO_FEED; p++)
            *p = generar(ctx);

        ok = fwrite(&usu, sizeof(usu), 1, pf) == 1;
    }

    fclose(pf);

    return ok;
}

/* Genera posteos.dat y usuarios_<tipo>.dat; la misma semilla reproduce el mismo lote */
int generarLote(unsigned long semilla, const char *tipo, int usuarios, int tamPool)
{
    tGenerarId generar = buscarTipo(tipo);
    char nombreUsu[64];
    tCtxLote ctx;
    int *p;
    int ok;

    if(!generar || usuarios < USUARIOS_MIN || usuarios > USUARIOS_MAX
            || tamPool < 1 || tamPool > CANT_POSTEOS)
    {
        printf("Parametros invalidos (usuarios %d a %d, pool 1 a %d)\n",
               USUARIOS_MIN, USUARIOS_MAX, CANT_POSTEOS);
        return PARAM_INVALIDO;
    }

    ctx.total = (unsigned long)usuarios * LARGO_FEED;
    ctx.contador = 0;
    ctx.tamPool = tamPool;

    if(generar == idOrdenado && ctx.total > CANT_POSTEOS)
    {
        printf("Lote ordenado: usuarios * %d no puede superar %d (maximo %d usuarios)\n",
               LARGO_FEED, CANT_POSTEOS, CANT_POSTEOS / LARGO_FEED);
        return PARAM_INVALIDO;
    }

    ctx.pool = malloc(tamPool * sizeof(int));
    if(!ctx.pool)
    {
        printf("Sin memoria\n");
        return SIN_MEM;
    }

    srand((unsigned)semilla);

    for(p = ctx.pool; p < ctx.pool + tamPool; p++)
        *p = (int)aleatorio(CANT_POSTEOS);

    snprintf(nombreUsu, sizeof(nombreUsu), FORMATO_USUARIOS, tipo);
    ok = generarPosteos(NOMBRE_POSTEOS)
         && generarUsuarios(nombreUsu, usuarios, generar, &ctx)
         && exportarATexto(NOMBRE_POSTEOS, sizeof(tPosteo), formatearPosteo) == TODO_OK
         && exportarATexto(nombreUsu, sizeof(tUsuario), formatearUsuario) == TODO_OK;

    free(ctx.pool);

    if(!ok)
    {
        printf("Error al escribir los archivos\n");
        return PARAM_INVALIDO;
    }

    printf("Lote '%s' (semilla %lu): %d posteos en %s, %d usuarios en %s (con sus .txt)\n",
           tipo, semilla, CANT_POSTEOS, NOMBRE_POSTEOS, usuarios, nombreUsu);

    return TODO_OK;
}
