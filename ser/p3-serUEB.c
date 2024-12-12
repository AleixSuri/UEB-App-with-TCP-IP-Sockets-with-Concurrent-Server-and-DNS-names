/**************************************************************************/
/*                                                                        */
/* L'aplicació UEB amb sockets TCP/IP                                     */
/* Fitxer serUEB.c que implementa la interfície aplicació-administrador   */
/* d'un servidor de l'aplicació UEB, sobre la capa d'aplicació de         */
/* (la part servidora de) UEB (fent crides a la interfície de la part     */
/* servidora de la capa UEB).                                             */
/*                                                                        */
/* Autors: Arnau Herrera i Aleix Suriñach                                 */
/* Data: desembre 2024                                                    */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <stdio.h> o #include "meu.h"     */
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include "p3-aUEBs.h"

/* Definició de constants, p.e.,                                          */

/* #define XYZ       1500                                                 */

/* Declaració de funcions INTERNES que es fan servir en aquest fitxer     */
/* (les  definicions d'aquestes funcions es troben més avall) per així    */
/* fer-les conegudes des d'aquí fins al final d'aquest fitxer, p.e.,      */

int AfegeixSck(int Sck, int *LlistaSck, int LongLlistaSck);
int TreuSck(int Sck, int *LlistaSck, int LongLlistaSck);
int LlegirFitxerCFG(int *fileLog, int *port, int *maxConTCP);
/* int FuncioInterna(arg1, arg2...);                                      */

int main(int argc, char *argv[])
{
    /* Declaració de variables, p.e., int n;                                 */
    char buffer[300];
    char TextRes[300];
    int sck;
    int portTCP;
    int maxConTCP;
    int *LlistaSck;
    int LongLlistaSck;
    // char arrelUEB[200] = {0};

    /* Expressions, estructures de control, crides a funcions, etc.          */

    // Obrir fitxer serUEB.log
    int fileLog = open("serUEB.log", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileLog == -1)
    {
        perror("\nError al obrir el fitxer per desar-lo.");
        exit(-1);
    }

    if (LlegirFitxerCFG(&fileLog, &portTCP, &maxConTCP) == -1)
    {
        exit(-1);
    }

    LongLlistaSck = 2 + maxConTCP; // Socket d'escolta + maxim de connexions
    LlistaSck = (int *)malloc(LongLlistaSck * sizeof(int));

    if (LlistaSck == NULL)
    {
        printf("malloc(), memòria no assignada\n");
        exit(-1);
    }

    int i;
    for (i = 0; i < LongLlistaSck; i++)
        LlistaSck[i] = -1;

    // Inicia Servidor
    if (UEBs_IniciaServ(&sck, portTCP, TextRes) == -1)
    {
        printf("UEBs_IniciaServ(): %s\n", TextRes);
        dprintf(fileLog, "UEBs_IniciaServ(): %s\n", TextRes);
        exit(-1);
    }
    printf("%s\nLimit de connexions simultanies: %d\n\n", TextRes, maxConTCP);

    AfegeixSck(sck, LlistaSck, LongLlistaSck); // Socket d'escolta

    // Bucle per esperar i rebre peticions
    while (1)
    {
        int socketAux = UEBs_HaArribatAlgunaCosaPerLlegir(LlistaSck, LongLlistaSck, TextRes);
        if (socketAux == -1)
        {
            printf("UEBs_HaArribatAlgunaCosaPerLlegir(): %s\n", TextRes);
            dprintf(fileLog, "UEBs_HaArribatAlgunaCosaPerLlegir(): %s\n", TextRes);
            exit(-1);
        }

        char IPloc[16];
        int portTCPloc;
        char IPrem[16];
        int portTCPrem;

        if (socketAux == sck)
        { // Socket d'escolta
            // Accepta connexio
            int sckCon = UEBs_AcceptaConnexio(sck, TextRes);
            if (sckCon == -1)
            {
                printf("UEBs_AcceptaConnexio(): %s\n", TextRes);
                dprintf(fileLog, "UEBs_AcceptaConnexio(): %s\n", TextRes);
                exit(-1);
            }

            if (AfegeixSck(sckCon, LlistaSck, LongLlistaSck) == -1)
            {                                        // Si l'ha acceptat però no hi ha prou espai a la llista
                UEBs_TancaConnexio(sckCon, TextRes); // Opció 1: fer close()
                printf("UEBc_TancaConnexio(): %s\n", TextRes);
                dprintf(fileLog, "UEBc_TancaConnexio(): %s\n", TextRes);
            }

            // Mostrar @sockets
            if (UEBs_TrobaAdrSckConnexio(sckCon, IPloc, &portTCPloc, IPrem, &portTCPrem, TextRes) == -1)
            {
                printf("UEBc_TrobaAdrSckConnexio(): %s\n", TextRes);
                dprintf(fileLog, "UEBc_TrobaAdrSckConnexio(): %s\n", TextRes);
                exit(-1);
            }
            printf("Connexió TCP @sck ser %s:%d @sck cli %s:%d\n", IPloc, portTCPloc, IPrem, portTCPrem);
            dprintf(fileLog, "Connexió TCP @sck ser %s:%d @sck cli %s:%d\n", IPloc, portTCPloc, IPrem, portTCPrem);
        }
        else if (socketAux == 0)
        { // Tancar Servidor
            printf("Vols tancar el servidor? (si/no)\n");
            int nBytes = read(0, buffer, sizeof(buffer));
            buffer[nBytes - 1] = '\0';

            if (strcmp(buffer, "si") == 0)
            {
                printf("Tancant servidor...\n");
                dprintf(fileLog, "Tancant servidor...\n");
                break;
            }
        }
        else
        { // Socket de connexio
            // Servir peticio
            char TextTemps[300];
            char NomFitx[200];
            char TipusPeticio[4];
            int bytesEnviats;
            int servPet = UEBs_ServeixPeticio(socketAux, TipusPeticio, NomFitx, TextRes, TextTemps);

            // Client tanca connexio
            if (servPet != -3)
            {
                printf("Petició %s del fitxer %s\n", TipusPeticio, NomFitx);
                dprintf(fileLog, "Petició %s del fitxer %s\n", TipusPeticio, NomFitx);
            }

            printf("UEBc_ServeixPeticio(): %s\n\n", TextRes);
            dprintf(fileLog, "UEBc_ServeixPeticio(): %s\n\n", TextRes);

            // Errors
            if (servPet == -1 || servPet == -2)
            {
                exit(-1);
            }

            // Tot correcte
            if (servPet == 0 || servPet == 1 || servPet == -4)
            {
                // Mostrar el temps
                printf("%s\n", TextTemps);
                dprintf(fileLog, "%s\n", TextTemps);
            }
        }
    }

    // Tanca connexions
    int k;
    for (k = 0; k < LongLlistaSck; k++)
    {
        if (LlistaSck[k] != 0 && LlistaSck[k] != -1)
        {
            UEBs_TancaConnexio(LlistaSck[k], TextRes);
            printf("UEBc_TancaConnexio(): %s\n", TextRes);
            dprintf(fileLog, "UEBc_TancaConnexio(): %s\n", TextRes);
        }
    }

    free(LlistaSck);
    close(fileLog);
}

/* Definició de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer. Les seves declaracions es troben */
/* a l'inici d'aquest fitxer.                                             */

/* Descripció de la funció, dels arguments, valors de retorn, etc.        */
/*int FuncioInterna(arg1, arg2...)
{

} */

/* Donada la llista d'identificadors de sockets “LlistaSck” (de longitud  */
/* “LongLlistaSck” sockets), hi busca una posició "lliure" (una amb un    */
/* contingut igual a -1) i hi escriu l'identificador de socket "Sck".     */
/*                                                                        */
/* "LlistaSck" és un vector d'int d'una longitud d'almenys LongLlistaSck. */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé;                                                       */
/* -1 si hi ha error.                                                     */
int AfegeixSck(int Sck, int *LlistaSck, int LongLlistaSck)
{
    // Recorrer totes les posicions fins trobar
    int index = 0, trobat = 0;
    while (index < LongLlistaSck && !trobat)
    {
        if (LlistaSck[index] != -1)
            index++;
        else
            trobat++;
    }

    // Si arriba al final del vector Error
    if (!trobat)
    {
        return -1;
    }

    // Posició trobada
    LlistaSck[index] = Sck;

    return 0;
}

/* Donada la llista d'identificadors de sockets “LlistaSck” (de longitud  */
/* “LongLlistaSck” sockets), hi busca la posició on hi ha l'identificador */
/* de socket "Sck" i la marca com "lliure" (hi escriu un contingut igual  */
/* a -1).                                                                 */
/*                                                                        */
/* "LlistaSck" és un vector d'int d'una longitud d'almenys LongLlistaSck. */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé;                                                       */
/* -1 si hi ha error.                                                     */
int TreuSck(int Sck, int *LlistaSck, int LongLlistaSck)
{
    // Recorrer totes les posicions fins trobar
    int index = 0, trobat = 0;
    while (index < LongLlistaSck && !trobat)
    {
        if (LlistaSck[index] != Sck)
            index++;
        else
            trobat++;
    }

    // Si arriba al final del vector Error
    if (!trobat)
    {
        return -1;
    }

    // Posició trobada
    LlistaSck[index] = -1;

    return 0;
}

/* Llegeix el fitxer de configuracio p3-serUEB.cfg i omple els            */
/* corresponents parametres                                               */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé;                                                       */
/* -1 si hi ha error.                                                     */
int LlegirFitxerCFG(int *fileLog, int *portTCP, int *maxConTCP)
{
    // Llegir fitxer config
    char linia[50], opcio[20], valor[20];
    FILE *fp;
    fp = fopen("p3-serUEB.cfg", "r");
    if (fp == NULL)
    {
        printf("Error en obrir el fitxer de configuració.\n");
        return -1;
    }

    while (fgets(linia, sizeof(linia), fp) != NULL)
    {
        if (sscanf(linia, "%s %s", opcio, valor) != 2)
        {
            printf("Format del fitxer cfg incorrecte.\n");
            dprintf(*fileLog, "Format del fitxer cfg incorrecte.\n");
            return -1;
        }

        // Comprovar l'opció que estem llegint
        if (strcmp(opcio, "#portTCP") == 0)
        {
            *portTCP = atoi(valor);
        }
        else if (strcmp(opcio, "#maxconTCP") == 0)
        {
            *maxConTCP = atoi(valor);
        }
        // else if (strcmp(opcio, "#Arrel") == 0) {
        //     strncpy(arrelUEB, valor, sizeof(arrelUEB) - 1);
        // }
    }
}