#include <stdio.h>
#include <stdlib.h>
#include "generadorLotes.h"
#include "testCruzado.h"
#include "simuladorTweener.h"
#include "datosEntrada.h"
#include "archivos.h"
#include "loteManual.h"

#define OP_SALIR        0
#define OP_CONCENTRADO  1
#define OP_DISPERSO     2
#define OP_ORDENADO     3
#define OP_TEST         4
#define OP_SIMULACION   5
#define OP_VER          6
#define OP_LOTE_MANUAL  7

#define LOTE_TODOS      4
#define LOTE_OTRO       5

#define ARCHIVO_POSTEOS     1
#define ARCHIVO_USUARIOS    2

#define SEMILLA_DEFECTO 42
#define VISTA_DEFECTO   10
#define LOTE_MANUAL_DEFECTO "manual"

static void mostrarMenu(void)
{
    printf("\n===== LRU Cache =====\n");
    printf("1. Generar lote concentrado\n");
    printf("2. Generar lote disperso\n");
    printf("3. Generar lote ordenado\n");
    printf("4. Test cruzado (Parte 1 vs Parte 2)\n");
    printf("5. Simulacion Tweener\n");
    printf("6. Ver contenido de un archivo\n");
    printf("7. Crear lote manual\n");
    printf("0. Salir\n");
}

/* Pide los parametros y genera un lote del tipo indicado */
static void opcionGenerar(const char *tipo, int pideTamPool)
{
    unsigned long semilla = leerNumero("Semilla", SEMILLA_DEFECTO);
    int usuarios = (int)leerNumero("Usuarios", USUARIOS_MIN);
    int tamPool = POOL_DEFECTO;

    if(pideTamPool)
        tamPool = (int)leerNumero("Posteos populares (pool)", POOL_DEFECTO);

    generarLote(semilla, tipo, usuarios, tamPool);
}

/* Elige el lote (uno de los generados, todos, o uno propio por nombre), pide las
   repeticiones y ejecuta la simulacion */
static void opcionSimular(void)
{
    static const char *tipos[] = {"concentrado", "disperso", "ordenado"};
    char otro[LARGO_NOMBRE];
    const char *elegido[1];
    const char **p = NULL;
    const char **fin = NULL;
    int lote = (int)leerNumero("Lote (1 concentrado, 2 disperso, 3 ordenado, 4 todos, 5 otro)", LOTE_TODOS);

    if(lote >= 1 && lote <= 3)
    {
        p = tipos + lote - 1;
        fin = p + 1;
    }
    else if(lote == LOTE_TODOS)
    {
        p = tipos;
        fin = tipos + 3;
    }
    else if(lote == LOTE_OTRO)
    {
        if(leerNombre("Nombre del lote", LOTE_MANUAL_DEFECTO, otro, sizeof(otro)))
        {
            *elegido = otro;
            p = elegido;
            fin = elegido + 1;
        }
    }
    else
        printf("Lote invalido.\n");

    if(p < fin)
    {
        int reps = (int)leerNumero("Repeticiones", 10);

        for(; p < fin; p++)
            ejecutarSimulacion(*p, reps);
    }
}

/* Muestra registros de posteos.dat o de un lote de usuarios */
static void opcionVer(void)
{
    char lote[LARGO_NOMBRE];
    int archivo = (int)leerNumero("Archivo (1 posteos, 2 usuarios)", ARCHIVO_POSTEOS);
    unsigned long desde;
    unsigned long cant;

    if(archivo != ARCHIVO_POSTEOS && archivo != ARCHIVO_USUARIOS)
    {
        printf("Archivo invalido.\n");
        return;
    }

    if(archivo == ARCHIVO_USUARIOS && !leerNombre("Lote", "concentrado", lote, sizeof(lote)))
        return;

    desde = leerNumero("Primer registro", 0);
    cant = leerNumero("Cantidad de registros", VISTA_DEFECTO);

    if(archivo == ARCHIVO_POSTEOS)
        verPosteos(desde, cant);
    else
        verUsuarios(lote, desde, cant);
}

/* Pide el nombre y crea un lote con los feeds escritos a mano */
static void opcionLoteManual(void)
{
    char lote[LARGO_NOMBRE];

    if(leerNombre("Nombre del lote", LOTE_MANUAL_DEFECTO, lote, sizeof(lote)))
        crearLoteManual(lote);
}

static void ejecutarOpcion(int op)
{
    if(op == OP_CONCENTRADO)
        opcionGenerar("concentrado", 1);
    else if(op == OP_DISPERSO)
        opcionGenerar("disperso", 0);
    else if(op == OP_ORDENADO)
        opcionGenerar("ordenado", 0);
    else if(op == OP_TEST)
        ejecutarTestCruzado();
    else if(op == OP_SIMULACION)
        opcionSimular();
    else if(op == OP_VER)
        opcionVer();
    else if(op == OP_LOTE_MANUAL)
        opcionLoteManual();
    else if(op != OP_SALIR)
        printf("Opcion invalida.\n");
}

int main(void)
{
    int op;

    do
    {
        mostrarMenu();
        op = (int)leerNumero("Opcion", OP_SALIR);   /* sin entrada: sale */
        ejecutarOpcion(op);
    }
    while(op != OP_SALIR);

    return 0;
}
