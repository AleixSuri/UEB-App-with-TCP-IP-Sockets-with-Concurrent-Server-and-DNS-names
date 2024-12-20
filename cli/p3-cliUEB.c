/**************************************************************************/
/*                                                                        */
/* L'aplicació UEB amb sockets TCP/IP                                     */
/* Fitxer cliUEB.c que implementa la interfície aplicació-usuari          */
/* d'un client de l'aplicació UEB, sobre la capa d'aplicació de           */
/* (la part client de) UEB (fent crides a la interfície de la part        */
/* client de la capa UEB).                                                */
/*                                                                        */
/* Autors: Arnau Herrera i Aleix Suriñach                                 */
/* Data: desembre 2024                                                    */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <stdio.h> o #include "meu.h"     */
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include "p3-aUEBc.h"
#include "p3-aDNSc.h"

/* Definició de constants, p.e.,                                          */

/* #define XYZ       1500                                                 */
#define MAX_FITX 10000
#define PORT_X_DEFECTE 6000

/* Declaració de funcions INTERNES que es fan servir en aquest fitxer     */
/* (les  definicions d'aquestes funcions es troben més avall) per així    */
/* fer-les conegudes des d'aquí fins al final d'aquest fitxer, p.e.,      */

int desferURI(const char *uri, char *esq, char *nom_host, int *port, char *nom_fitx);
void controlarPeticio(int sckCon, char *IPser, int portTCPser, char *NomFitx);
void obtenirFitxer(int sckCon, char *NomFitx, char *Fitx, int *LongFitx, char *TextRes);
void guardarFitxer(char *NomFitx, char *Fitx, int LongFitx);
void mostrarFitxer(char *Fitx, int LongFitx);
void mostrarTemps(struct timespec start, struct timespec end);

int main(int argc, char *argv[])
{
    char TextRes[300];
    int sckCon;
    int op;

    printf("Vols sortir o fer una petició? (0-Sortir, 1-Petició):\n");
    scanf("%d", &op);

    if (op == 1)
    {
        char uri[400];
        char esq[50];
        char host[300];
        char NomFitx[200];
        int portTCPser;
        char IPser[16];

        printf("Entra l'URI del lloc UEB: \n");
        scanf("%s", uri);

        if (desferURI(uri, esq, host, &portTCPser, NomFitx) == 3)
        {
            portTCPser = PORT_X_DEFECTE;
        }
        NomFitx[strlen(NomFitx)] = '\0';

        if (strcmp(esq, "pueb\0") != 0)
        {
            printf("ERROR: no es correcte el format de l'URI\n");
            printf("pueb://[nomDNS]{:[port] o res}/...\n");
            exit(-1);
        }

        if (DNSc_ResolDNSaIP(host, IPser, TextRes) == -1)
        {
            printf("DNSc_ResolDNSaIP(): %s\n", TextRes);
            exit(-1);
        }

        if ((sckCon = UEBc_DemanaConnexio(IPser, portTCPser, TextRes)) == -1)
        {
            printf("UEBc_DemanaConnexio(): %s\n", TextRes);
            exit(-1);
        }

        controlarPeticio(sckCon, IPser, portTCPser, NomFitx);

        if (UEBc_TancaConnexio(sckCon, TextRes) == -1)
        {
            printf("UEBc_TancaConnexio(): %s", TextRes);
            exit(-1);
        }
    }

    return 0;
}

/* Fa tota la gestió per obtenir, mostrar i guardar el fitxer demanat     */
void controlarPeticio(int sckCon, char *IPser, int portTCPser, char *NomFitx)
{
    char TextRes[300];
    char Fitx[MAX_FITX];
    int LongFitx;
    int op = 1;

    while (op != 0)
    {
        char IPloc[16];
        int portTCPloc;
        char IPrem[16];
        int portTCPrem;
        if (UEBc_TrobaAdrSckConnexio(sckCon, IPloc, &portTCPloc, IPrem, &portTCPrem, TextRes) == -1)
        {
            printf("UEBc_TrobaAdrSckConnexio(): %s\n", TextRes);
            exit(-1);
        }
        printf("\nPetició OBT, @IP:#portTCP servidor %s:%d, fitxer %s\n", IPser, portTCPser, NomFitx);
        printf("Connexió TCP @sck cli %s:%d @sck ser %s:%d\n", IPloc, portTCPloc, IPrem, portTCPrem);

        struct timespec start, end;
        gettimeofday(&start, NULL);

        obtenirFitxer(sckCon, NomFitx, Fitx, &LongFitx, TextRes);

        gettimeofday(&end, NULL);

        mostrarFitxer(Fitx, LongFitx);
        mostrarTemps(start, end);

        if (LongFitx > 0)
        {
            guardarFitxer(NomFitx, Fitx, LongFitx);
        }

        printf("\n\nVols obtenir un fitxer o acabar? (1-Obtenir, 0-Acabar):\n");
        scanf("%d", &op);
        if (op == 1)
        {
            char uri[400];
            char esq[50];
            char host[300];
            int portTCPserAux = portTCPser;
            char IPserAux[16];
            strcpy(IPserAux, IPser);

            memset(TextRes, 0, sizeof(TextRes));
            memset(IPser, 0, sizeof(IPser));
            memset(NomFitx, 0, sizeof(NomFitx));
            memset(Fitx, 0, sizeof(Fitx));
            memset(uri, 0, sizeof(uri));

            printf("Entra l'URI del lloc UEB: \n");
            scanf("%s", uri);

            if (desferURI(uri, esq, host, &portTCPser, NomFitx) == 3)
            {
                portTCPser = PORT_X_DEFECTE;
            }
            NomFitx[strlen(NomFitx)] = '\0';

            if (strcmp(esq, "pueb\0") != 0)
            {
                printf("ERROR: no es correcte el format de l'URI\n");
                printf("pueb://[nomDNS]{:[port] o res}/...\n");
                exit(-1);
            }

            if (DNSc_ResolDNSaIP(host, IPser, TextRes) == -1)
            {
                printf("DNSc_ResolDNSaIP(): %s\n", TextRes);
                exit(-1);
            }

            if ((strcmp(IPser, IPserAux) != 0) || (portTCPser != portTCPserAux))
            {
                if (UEBc_TancaConnexio(sckCon, TextRes) == -1)
                {
                    printf("UEBc_TancaConnexio(): %s", TextRes);
                    exit(-1);
                }

                if ((sckCon = UEBc_DemanaConnexio(IPser, portTCPser, TextRes)) == -1)
                {
                    printf("UEBc_DemanaConnexio(): %s\n", TextRes);
                    exit(-1);
                }
            }
        }
    }
}

/* Obte el fitxer demanat de nom NomFitx, omplint Fitx i LongFitx,        */
/* el contingut i la mida d'aquest respectivament                         */
void obtenirFitxer(int sckCon, char *NomFitx, char *Fitx, int *LongFitx, char *TextRes)
{
    char nomAux[200];
    memset(nomAux, 0, sizeof(nomAux));
    memcpy(nomAux, NomFitx, (int)strlen(NomFitx));
    int obtFit = UEBc_ObteFitxer(sckCon, nomAux, Fitx, LongFitx, TextRes);
    if (obtFit == -1 || obtFit == -2)
    {
        printf("UEBc_ObteFitxer(): %s\n", TextRes);
        exit(-1);
    }
    else if (obtFit == -3)
    {
        printf("UEBc_ObteFitxer(): %s\n", TextRes);
        exit(0);
    }
    else
    {
        printf("UEBc_ObteFitxer(): %s\n\n", TextRes);
    }
}

/* Desa el fitxer de nom NomFitx, amb contingut Fitx i de mida LongFitx   */
/* al directori on s'està executant el client                             */
/* si ja existeix, es sobreescriu, si no es crea                          */
void guardarFitxer(char *NomFitx, char *Fitx, int LongFitx)
{
    char nomFitxer[200];
    strcpy(nomFitxer, &NomFitx[1]); // treure '/' NomFitxer

    if (strlen(nomFitxer) == 0 || nomFitxer[strlen(nomFitxer) - 1] == '/')
    {
        strcat(nomFitxer, "index.html");
    }

    int file = open(nomFitxer, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (file == -1)
    {
        perror("\nError al obrir el fitxer per desar-lo.");
        exit(-1);
    }

    int n = write(file, Fitx, LongFitx);
    if (n == -1)
    {
        perror("\nError al escriure el fitxer per desar-lo.");
        close(file);
        exit(-1);
    }

    if (close(file) == -1)
    {
        perror("\nError al tancar el fitxer desat.");
        exit(-1);
    }
}

/* Mostra el fitxer de contingut Fitx i mida LongFitx                     */
void mostrarFitxer(char *Fitx, int LongFitx)
{
    if (write(1, Fitx, LongFitx) == -1)
    {
        perror("Error en mostrar el contingut del fitxer.");
        exit(-1);
    }
}

/* Mostra el temps des de l'inici (start) fins el final (end)             */
void mostrarTemps(struct timespec start, struct timespec end)
{
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    double elapsed = seconds + nanoseconds * 1e-9;
    printf("\n\nTemps de resposta: %.9f segons \n", elapsed);
}

/* Desfà l'URI "uri" en les seves parts: l'esquema (protocol) "esq", el   */
/* nom DNS (o l'@IP) "nom_host", el número de port "port" i el nom del    */
/* fitxer "nom_fitxer".                                                   */
/*                                                                        */
/* L'URI ha de tenir la forma "esq://nom_host:port/nom_fitxer" o bé       */
/* sense el número de port "esq://nom_host/nom_fitxer", i llavors port    */
/* s'emplena amb el valor 0 (la resta de casos no es contemplen).         */
/*                                                                        */
/* "uri", "esq", "nom_host" i "nom_fitxer" són "strings" de C (vector de  */
/* chars imprimibles acabat en '\0') d'una longitud suficient.            */
/*                                                                        */
/* Retorna:                                                               */
/*  el nombre de parts de l'URI que s'han assignat (4 si l'URI tenia      */
/*  número de port o 3 si no en tenia).                                   */
int desferURI(const char *uri, char *esq, char *nom_host, int *port, char *nom_fitx)
{
    int nassignats = 0;
    char port_str[100];

    strcpy(esq, "");
    strcpy(nom_host, "");
    *port = 0;
    strcpy(nom_fitx, "");

    nassignats = sscanf(uri, "%[^:]://%[^:]:%[^/]%s", esq, nom_host, port_str, nom_fitx);

    /*URIs amb #port, p.e., esq://host:port/fitx, 4 valors assignats */
    if (nassignats == 4)
    {
        *port = atoi(port_str); // Convertim el port a un enter
        return nassignats;
    }

    /* URIs sense #port, p.e., esq://host/fitx, 2 valors assignats; */
    /* llavors es fa port = 0 i una nova assignació                 */
    if (nassignats == 2)
    {
        nassignats = sscanf(uri, "%[^:]://%[^/]%s", esq, nom_host, nom_fitx);
        *port = 0; // Assignem 0 al port si no està especificat
        return nassignats;
    }

    return nassignats;
}