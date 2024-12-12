/**************************************************************************/
/*                                                                        */
/* L'aplicació UEB amb sockets TCP/IP                                     */
/* Fitxer capçalera de aDNSc.c                                            */
/*                                                                        */
/* Autors: Arnau Herrera i Aleix Suriñach                                 */
/* Data: desembre 2024                                                    */
/*                                                                        */
/**************************************************************************/

/* Declaració de funcions EXTERNES de aDNSc.c, és a dir, d'aquelles       */
/* funcions que es faran servir en un altre fitxer extern a aDNSc.c,      */
/* p.e., int DNSc_FuncioExterna(arg1, arg2...) { }                        */
/* El fitxer extern farà un #include del fitxer aDNSc.h a l'inici, i      */
/* així les funcions seran conegudes en ell.                              */
/* En termes de capes de l'aplicació, aquest conjunt de funcions          */
/* EXTERNES formen la interfície de la capa DNS, en la part client.       */

int DNSc_ResolDNSaIP(const char *NomDNS, char *IP, char *TextRes);
/* int DNSc_FuncioExterna(arg1, arg2...);                                 */
