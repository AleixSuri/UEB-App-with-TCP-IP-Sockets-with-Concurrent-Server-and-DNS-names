/**************************************************************************/
/*                                                                        */
/* L'aplicació UEB amb sockets TCP/IP                                     */
/* Fitxer aDNSc.c que "implementa" la capa d'aplicació DNS (part          */
/* client), o més ben dit, que encapsula les crides a sistema de la       */
/* part DNS, en unes altres funcions més simples i entendedores: la       */
/* "nova" interfície de la capa DNS, en la part client.                   */
/*                                                                        */
/* Autors: Arnau Herrera i Aleix Suriñach                                 */
/* Data: desembre 2024                                                    */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
/*  (si les funcions EXTERNES es cridessin entre elles, faria falta fer   */
/*   un #include del propi fitxer capçalera)                              */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//* Definició de constants, p.e.,                                         */

/* #define XYZ       1500                                                 */

/* Declaració de funcions INTERNES que es fan servir en aquest fitxer     */
/* (les  definicions d'aquestes funcions es troben més avall) per així    */
/* fer-les conegudes des d'aquí fins al final d'aquest fitxer, p.e.,      */

/* int FuncioInterna(arg1, arg2...);                                      */

/* Definició de funcions EXTERNES, és a dir, d'aquelles que es cridaran   */
/* des d'altres fitxers, p.e., int DNSc_FuncioExterna(arg1, arg2...) { }  */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa DNS, en la part client.                */

/* Donat el nom DNS "NomDNS" obté la corresponent @IP i l'escriu a "IP"   */
/* Escriu un text que descriu el resultat de la funció a "TextRes".       */
/*                                                                        */
/* "NomDNS" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud qualsevol.                                        */
/* "IP" és un "string" de C (vector de chars imprimibles acabat en        */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* "TextRes" és un "string" de C (vector de chars imprimibles acabat en   */
/* '\0') d'una longitud màxima de 200 chars (incloent '\0').              */
/*                                                                        */
/* Retorna:                                                               */
/*  0 si tot va bé;                                                       */
/* -1 si hi ha un error.                                                  */
int DNSc_ResolDNSaIP(const char *NomDNS, char *IP, char *TextRes)
{
    struct hostent *dadesHOST;
    struct in_addr adrHOST;

    /* Es fa la petició de resolució DNS */
    dadesHOST = gethostbyname(NomDNS);
    if (dadesHOST == NULL)
    {
        sprintf(TextRes, "gethostbyname(): %s", hstrerror(errno));
        return -1;
    }
    else
    {
        adrHOST.s_addr = *((unsigned long *)dadesHOST->h_addr_list[0]);
        strcpy(IP, (char *)inet_ntoa(adrHOST));
        sprintf(TextRes, "Tot bé");
        return 0;
    }
}

/* Si ho creieu convenient, feu altres funcions EXTERNES                  */

/* Descripció de la funció, dels arguments, valors de retorn, etc.        */
/* int DNSc_FuncioExterna(arg1, arg2...)
{

} */

/* Definició de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer. Les seves declaracions es        */
/* troben a l'inici d'aquest fitxer.                                      */

/* Descripció de la funció, dels arguments, valors de retorn, etc.        */
/* int FuncioInterna(arg1, arg2...)
{

} */
