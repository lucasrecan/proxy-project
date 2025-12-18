#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include  <sys/select.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port

int read_line(int fd, char *buffer, int max_len) {
    int n, rc;
    char c;
    for (n = 0; n < max_len - 1; n++) {
        rc = read(fd, &c, 1);
        if (rc == 1) {
            buffer[n] = c;
            if (c == '\n') {
                n++; 
                break;
            }
        } else if (rc == 0) { 
            if (n == 0) return 0; 
            else break; 
        } else {
            return -1; 
        }
    }
    buffer[n] = '\0'; 
    return n;
}

int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les 
				                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
    
    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
				                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

	len = sizeof(struct sockaddr_storage);
     // Attente connexion du client
     // Lorsque demande de connexion, creation d'une socket de communication avec le client
     descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
     if (descSockCOM == -1){
         perror("Erreur accept\n");
         exit(6);
     }
    // Echange de données avec le client connecté

    /*****
     * Testez de mettre 220 devant BLABLABLA ...
     * **/
    // strcpy(buffer, "BLABLABLA\n");
    // write(descSockCOM, buffer, strlen(buffer));

    /*******
     * 
     * A vous de continuer !
     * 
     * *****/

    // Envoi du message d'accueil initial au client
    char *greeting = "220 Proxy ready\r\n";
    write(descSockCOM, greeting, strlen(greeting));

    // Lecture de la commande USER du client
    if (read_line(descSockCOM, buffer, MAXBUFFERLEN) <= 0) {
        perror("Error reading USER command");
        close(descSockCOM);
        // Passer à l'itération suivante (on implémentera la boucle plus tard)
        return 0; 
    }

    char login[MAXHOSTLEN];
    char host[MAXHOSTLEN];
    char *at_sign = strchr(buffer, '@');

    if (strncmp(buffer, "USER ", 5) == 0 && at_sign != NULL) {
        // Analyse de l'utilisateur et de l'hôte
        *at_sign = '\0'; // Séparation de la chaîne au caractère '@'
        strcpy(login, buffer + 5); // Sauter le préfixe "USER "
        // Le reste du buffer après '@' est l'hôte, mais il peut contenir \r\n
        strcpy(host, at_sign + 1);
        
        // Suppression des caractères \r\n de l'hôte
        char *crlf = strstr(host, "\r\n");
        if (crlf) *crlf = '\0';
        else {
             char *lf = strchr(host, '\n');
             if (lf) *lf = '\0';
        }

        printf("Detected USER: %s, HOST: %s\n", login, host);

        // Connexion au serveur réel
        int descSockServer;
        if (connect2Server(host, "21", &descSockServer) == -1) {
             perror("Error connecting to real server");
             close(descSockCOM);
             return 0;
        }

        // Lecture du message d'accueil 220 du serveur
        char server_buff[MAXBUFFERLEN];
        read_line(descSockServer, server_buff, MAXBUFFERLEN);
        printf("Server Banner (discarded): %s", server_buff);

        // Envoi de la commande USER au serveur réel
        snprintf(buffer, MAXBUFFERLEN, "USER %s\r\n", login);
        write(descSockServer, buffer, strlen(buffer));

        // boucle principale pour relayer les messages
        fd_set rset;
        int maxfd = (descSockCOM > descSockServer ? descSockCOM : descSockServer) + 1;

        while (1) {
            FD_ZERO(&rset);
            FD_SET(descSockCOM, &rset);
            FD_SET(descSockServer, &rset);

            if (select(maxfd, &rset, NULL, NULL, NULL) < 0) {
                 perror("select error");
                 break;
            }

            // Client -> Proxy -> Serveur
            if (FD_ISSET(descSockCOM, &rset)) {
                int n = read(descSockCOM, buffer, MAXBUFFERLEN);
                if (n <= 0) break; // Déconnexion du client
                
                // Écriture vers le serveur
                write(descSockServer, buffer, n);
            }

            // Serveur -> Proxy -> Client
            if (FD_ISSET(descSockServer, &rset)) {
                int n = read(descSockServer, buffer, MAXBUFFERLEN);
                if (n <= 0) break; // Déconnexion du serveur
                
                // Écriture vers le client
                write(descSockCOM, buffer, n);
            }
        }
        
        close(descSockServer);
        close(descSockCOM);
        printf("Session closed.\n");
        exit(0); // un seul client

    } else {
        char *err = "500 Syntax error, command unrecognized or missing @host\r\n";
        write(descSockCOM, err, strlen(err));
        close(descSockCOM);
    }

    close(descSockCOM);
    close(descSockRDV);
}

