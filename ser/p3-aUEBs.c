/**************************************************************************/
/*                                                                        */
/* L'aplicació UEB amb sockets TCP/IP                                     */
/* Fitxer aUEB.c que implementa la capa d'aplicació de UEB, sobre la      */
/* cap de transport TCP (fent crides a la "nova" interfície de la         */
/* capa TCP o "nova" interfície de sockets TCP), en la part servidora.    */
/*                                                                        */
/* Autors: Arnau Herrera i Aleix Suriñach                                 */
/* Data: desembre 2024                                                    */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
/*  (si les funcions externes es cridessin entre elles, faria falta fer   */
/*   un #include del propi fitxer capçalera)                              */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "p3-tTCP.h"
#include "p3-aUEBs.h"

/* Definició de constants, p.e.,                                          */

/* #define XYZ       1500                                                 */

/* Declaració de funcions INTERNES que es fan servir en aquest fitxer     */
/* (les  definicions d'aquestes funcions es troben més avall) per així    */
/* fer-les conegudes des d'aquí fins al final d'aquest fitxer, p.e.,      */

/* int FuncioInterna(arg1, arg2...);                                      */

int ConstiEnvMis(int SckCon, const char *tipus, const char *info1, int long1);
int RepiDesconstMis(int SckCon, char *tipus, char *info1, int *long1);

/* Definició de funcions EXTERNES, és a dir, d'aquelles que es cridaran   */
/* des d'altres fitxers, p.e., int UEBs_FuncioExterna(arg1, arg2...) { }  */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa UEB, en la part servidora.             */

/* Inicia el S UEB: crea un nou socket TCP "servidor" a una @IP local     */
/* qualsevol i al #port TCP “portTCPser”. Escriu l'identificador del      */
/* socket creat a "SckEsc".                                               */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé;                                                       */
/* -1 si hi ha un error en la interfície de sockets.                      */
int UEBs_IniciaServ(int *SckEsc, int portTCPser, char *TextRes)
{
    int CodiRes;
    int intent = 0;
    // 5 intents per trobar un socket lliure
    while (intent < 5)
    {
        *SckEsc = TCP_CreaSockServidor("0.0.0.0", portTCPser);

        // Socket creat correctament
        if (*SckEsc >= 0)
        {
            // Si el socket es crea correctament, surt del bucle
            sprintf(TextRes, "Servidor UEB iniciat al #portTCP: %d", portTCPser);
            return *SckEsc;
        }

        printf("Port %d en ús, provant amb el següent...\n", portTCPser);
        portTCPser += 101; // Incrementa el port i prova de nou
        intent++;          // Incrementa el comptador d'intents
    }

    sprintf(TextRes, "TCP_CreaSockServidor(): %s", T_ObteTextRes(&CodiRes));
    return -1;
}

/* Accepta una connexió d'un C UEB que arriba a través del socket TCP     */
/* "servidor" d'identificador "SckEsc".                                   */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*  l'identificador del socket TCP connectat si tot va bé;                */
/* -1 si hi ha un error a la interfície de sockets.                       */
int UEBs_AcceptaConnexio(int SckEsc, char *TextRes)
{
    int CodiRes;
    char ipRem[16];
    int portRem;
    int res = TCP_AcceptaConnexio(SckEsc, ipRem, &portRem);
    if (res == -1)
    {
        sprintf(TextRes, "TCP_AcceptaConnexio(): %s", T_ObteTextRes(&CodiRes));
        TCP_TancaSock(res);
        return -1;
    }

    sprintf(TextRes, "Connexió establerta amb èxit\0");
    return res;
}

/* Serveix una petició UEB d'un C a través de la connexió TCP             */
/* d'identificador "SckCon". A "TipusPeticio" hi escriu el tipus de       */
/* petició (p.e., OBT) i a "NomFitx" el nom del fitxer de la petició.     */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "TipusPeticio" és un "string" de C (vector de chars imprimibles acabat */
/* en '\0') d'una longitud de 4 chars (incloent '\0').                    */
/* "NomFitx" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud <= 10000 chars (incloent '\0').                   */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si el fitxer existeix al servidor;                                  */
/*  1 la petició és ERRònia (p.e., el fitxer no existeix);                */
/* -1 si hi ha un error a la interfície de sockets;                       */
/* -2 si protocol és incorrecte (longitud camps, tipus de peticio, etc.); */
/* -3 si l'altra part tanca la connexió;                                  */
/* -4 si hi ha problemes amb el fitxer de la petició (p.e., nomfitxer no  */
/*  comença per /, fitxer no es pot llegir, fitxer massa gran, etc.).     */
int UEBs_ServeixPeticio(int SckCon, char *TipusPeticio, char *NomFitx, char *TextRes, char *TextTemps)
{
    int CodiRes;
    int long1; // mida caracters nom fitxer
    // Variables per mesurar el temps
    struct timespec start, end;

    // Rep missatge PUEB OBT
    int res = RepiDesconstMis(SckCon, TipusPeticio, NomFitx, &long1);
    TipusPeticio[3] = '\0';
    if (res == -1)
    {
        sprintf(TextRes, "TCP_Rep(): %s", T_ObteTextRes(&CodiRes));
        return -1;
    }
    else if (res == -2)
    {
        sprintf(TextRes, "Protocol incorrecte.");
        return -2;
    }
    else if (res == -3)
    {
        sprintf(TextRes, "El client ha tancat la connexió.");
        return -3;
    }

    // Llegir arrel llocUEB
    char arrelUEB[200] = {0};
    char linia[50], opcio[30], valor[40];
    FILE *fp;
    fp = fopen("p2-serUEB.cfg", "r");
    if (fp == NULL)
    {
        perror("Error en obrir el fitxer de configuració.\n");
        return -1;
    }
    while (fgets(linia, sizeof(linia), fp) != NULL)
    {
        if (sscanf(linia, "%s %s", opcio, valor) != 2)
        {
            perror("Format del fitxer cfg incorrecte.\n");
            return -1;
        }

        // Comprovar l'opció que estem llegint
        if (strcmp(opcio, "#Arrel") == 0)
        {
            strncpy(arrelUEB, valor, sizeof(arrelUEB) - 1);
        }
    }

    // Obre fitxer
    char tipusEnv[3];
    int longEnv;
    char infoEnv[10000];
    int file;
    if (NomFitx[0] != '/')
    { // No comença per /
        sprintf(TextRes, "El nom del fitxer ha de començar amb '/'");
        memcpy(tipusEnv, "ERR", 3);
        longEnv = (int)strlen("fitxer no comença per /\0");
        memcpy(infoEnv, "fitxer no comença per /", longEnv);

        file = -4;
    }
    else
    { // Nom fitxer comença per /
        // Buscar i treure informació del fitxer NomFitxer rebut
        char contingutFitxer[10000];
        char nomFitxer[200];

        NomFitx[long1] = '\0';
        memcpy(nomFitxer, arrelUEB + 1, strlen(arrelUEB) - 1);
        memcpy(nomFitxer + strlen(nomFitxer), NomFitx, long1 + 1);

        file = open(nomFitxer, O_RDONLY);
        if (file == -1)
        { // fitxer no existeix
            memcpy(tipusEnv, "ERR", 3);
            longEnv = (int)strlen("1 fitxer no trobat");
            memcpy(infoEnv, "1 fitxer no trobat", longEnv);
        }
        else
        { // fitxer existeix
            int bytesFitxer = read(file, contingutFitxer, 10000);
            if (bytesFitxer == -1)
            {
                sprintf(TextRes, "Fitxer no es pot llegir correctament.");
                close(file);
                return -4;
            }

            // fitxer molt gran
            if (bytesFitxer > 9999)
            {
                sprintf(TextRes, "Fitxer massa gran per ser enviat.");
                memcpy(tipusEnv, "ERR", 3);
                longEnv = (int)strlen("fitxer massa gran\0");
                memcpy(infoEnv, "fitxer massa gran", longEnv);
                file = -4;
            }
            else
            {
                memcpy(tipusEnv, "COR", 3);
                longEnv = bytesFitxer;
                memcpy(infoEnv, contingutFitxer, longEnv);
                close(file);
            }
        }
    }

    // Enviar missatge PUEB amb contingut del fitxer o errors
    gettimeofday(&start, NULL);
    int res2 = ConstiEnvMis(SckCon, tipusEnv, infoEnv, longEnv);
    gettimeofday(&end, NULL);

    // Variables mesura temps
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    double elapsed = seconds + nanoseconds * 1e-9;
    double vef = (longEnv * 8) / elapsed;

    // Guardar el temps i velocitat
    sprintf(TextTemps, "Temps d'enviament: %.9f segons i %.2f bps\n", elapsed, vef);

    if (res2 == -1)
    {
        sprintf(TextRes, "TCP_Envia(): %s", T_ObteTextRes(&CodiRes));
        return -1;
    }
    else if (res2 == -2)
    {
        sprintf(TextRes, "Protocol incorrecte.");
        return -2;
    }

    // Canvi de missatge depenen si existeix o no o massa gran
    if (file == -1)
    {
        sprintf(TextRes, "Tot bé, fitxer no existeix.");
        return 1;
    }
    else if (file == -4)
    {
        return -4;
    }
    else
    {
        sprintf(TextRes, "Tot bé, el fitxer existeix al servidor.");
        return 0;
    }
}

/* Tanca la connexió TCP d'identificador "SckCon".                        */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*   0 si tot va bé;                                                      */
/*  -1 si hi ha un error a la interfície de sockets.                      */
int UEBs_TancaConnexio(int SckCon, char *TextRes)
{
    int CodiRes;
    int res = TCP_TancaSock(SckCon);
    if (res == -1)
    {
        sprintf(TextRes, "TCP_TancaSock(): %s", T_ObteTextRes(&CodiRes));
        return -1;
    }

    strcpy(TextRes, "Socket servidor tancat correctament\0");
    return res;
}

/* Donat el socket TCP “connectat” d’identificador “SckCon”, troba        */
/* l’adreça del socket local, omplint “IPloc” i “portTCPloc” amb          */
/* respectivament, la seva @IP i #port TCP, i troba l'adreça del socket   */
/* remot amb qui està connectat, omplint “IPrem” i  “portTCPrem” amb      */
/* respectivament, la seva @IP i #port TCP.                               */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "IPloc" i "IPrem" són "strings" de C (vector de chars imprimibles      */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*   0 si tot va bé;                                                      */
/*  -1 si hi ha un error a la interfície de sockets.                      */
int UEBs_TrobaAdrSckConnexio(int SckCon, char *IPloc, int *portTCPloc, char *IPrem, int *portTCPrem, char *TextRes)
{
    int CodiRes;
    int res1 = TCP_TrobaAdrSockLoc(SckCon, IPloc, portTCPloc);
    int res2 = TCP_TrobaAdrSockRem(SckCon, IPrem, portTCPrem);

    if ((res1 == -1))
    {
        sprintf(TextRes, "TCP_TrobaAdrSockLoc(): %s", T_ObteTextRes(&CodiRes));
        return -1;
    }

    if ((res2 == -1))
    {
        sprintf(TextRes, "TCP_TrobaAdrSockRem(): %s", T_ObteTextRes(&CodiRes));
        return -1;
    }

    sprintf(TextRes, "Tot bé\0");
    return 0;
}

/* Examina simultàniament i sense límit de temps (una espera indefinida)  */
/* els sockets (poden ser TCP, UDP i  teclat -stdin-) amb identificadors  */
/* en la llista “LlistaSck” (de longitud “LongLlistaSck” sockets) per     */
/* saber si hi ha arribat alguna cosa per ser llegida, excepte aquells    */
/* que tinguin identificadors igual a -1.                                 */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "LlistaSck" és un vector d'int d'una longitud d'almenys LongLlistaSck. */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*  l'identificador del socket a través del qual ha arribat alguna cosa;  */
/*  -1 si hi ha error.                                                    */
int UEBs_HaArribatAlgunaCosaPerLlegir(const int *LlistaSck, int LongLlistaSck, char *TextRes)
{
}

/* Si ho creieu convenient, feu altres funcions EXTERNES                  */

/* Descripció de la funció, dels arguments, valors de retorn, etc.        */
/* int UEBs_FuncioExterna(arg1, arg2...)
{

} */

/* Definició de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer. Les seves declaracions es        */
/* troben a l'inici d'aquest fitxer.                                      */

/* Descripció de la funció, dels arguments, valors de retorn, etc.        */
/* int FuncioInterna(arg1, arg2...)
{

} */

/* "Construeix" un missatge de PUEB a partir dels seus camps tipus,       */
/* long1 i info1, escrits, respectivament a "tipus", "long1" i "info1"    */
/* (que té una longitud de "long1" bytes), i l'envia a través del         */
/* socket TCP “connectat” d’identificador “SckCon”.                       */
/*                                                                        */
/* "tipus" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud de 4 chars (incloent '\0').                       */
/* "info1" és un vector de chars (bytes) qualsevol (recordeu que en C,    */
/* un char és un enter de 8 bits) d'una longitud <= 9999 bytes.           */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé;                                                       */
/* -1 si hi ha un error a la interfície de sockets;                       */
/* -2 si protocol és incorrecte (longitud camps, tipus de peticio).       */
int ConstiEnvMis(int SckCon, const char *tipus, const char *info1, int long1)
{
    // Ajuntar missatge PUEB
    char SeqBytes[10006];
    if ((long1 > 9999) || (compara_vectors(tipus, "OBT", 3) == 0))
    {
        return -2;
    }

    char longAux[4];
    sprintf(longAux, "%.4d", long1);
    memcpy(longAux, longAux, 4);

    memcpy(SeqBytes, tipus, 3);
    memcpy(SeqBytes + 3, longAux, 4);
    memcpy(SeqBytes + 7, info1, long1);

    // Envia missatge
    if (TCP_Envia(SckCon, SeqBytes, 3 + 4 + long1) == -1)
    {
        return -1;
    }

    return 0;
}

/* Rep a través del socket TCP “connectat” d’identificador “SckCon” un    */
/* missatge de PUEB i el "desconstrueix", és a dir, obté els seus camps   */
/* tipus, long1 i info1, que escriu, respectivament a "tipus", "long1"    */
/* i "info1" (que té una longitud de "long1" bytes).                      */
/*                                                                        */
/* "tipus" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud de 4 chars (incloent '\0').                       */
/* "info1" és un vector de chars (bytes) qualsevol (recordeu que en C,    */
/* un char és un enter de 8 bits) d'una longitud <= 9999 bytes.           */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé,                                                       */
/* -1 si hi ha un error a la interfície de sockets;                       */
/* -2 si protocol és incorrecte (longitud camps, tipus de peticio);       */
/* -3 si l'altra part tanca la connexió.                                  */
int RepiDesconstMis(int SckCon, char *tipus, char *info1, int *long1)
{
    // Desglossa missatge PUEB
    char SeqBytes[10006];
    int bytesRebuts = TCP_Rep(SckCon, SeqBytes, sizeof(SeqBytes));

    if (bytesRebuts == -1)
    {
        return -1;
    }
    else if (bytesRebuts == 0)
    {
        return -3;
    }
    else
    {
        memcpy(tipus, SeqBytes, 3);

        char longAux[4];
        memcpy(longAux, SeqBytes + 3, 4);
        *long1 = atoi(longAux);

        if ((bytesRebuts < 8) || (compara_vectors(tipus, "OBT", 3) == 1) || ((bytesRebuts - 7) != *long1))
        {
            return -2;
        }
        else
        {
            memcpy(info1, SeqBytes + 7, *long1);
            return 0;
        }
    }
}

/* Compara dos vectors de caracters                                       */
/*                                                                        */
/* Retorna:                                                               */
/*  0 son iguals                                                          */
/*  1 no son iguals                                                       */
int compara_vectors(const char *vec1, const char *vec2, int mida)
{
    int i;
    for (i = 0; i < mida; i++)
    {
        if (vec1[i] != vec2[i])
        {
            return 1; // Si alguna posició és diferent, retorna false
        }
    }
    return 0; // Si no hi ha cap diferència, retorna true
}
