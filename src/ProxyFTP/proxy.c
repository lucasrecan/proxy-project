#define _POSIX_C_SOURCE 200112L
#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include  <sys/select.h>
#include  <signal.h>
#include  <sys/wait.h>
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

// Parse la commande PORT et extrait l'IP et le port du client
// Format: PORT h1,h2,h3,h4,p1,p2
// Retourne 0 si succès, -1 si erreur
int parse_port_command(const char *cmd, char *client_ip, int *client_port) {
    int h1, h2, h3, h4, p1, p2;
    
    // Chercher le début des paramètres (après "PORT ")
    const char *params = cmd + 5; // Sauter "PORT "
    
    if (sscanf(params, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        return -1;
    }
    
    // Construire l'adresse IP
    snprintf(client_ip, MAXHOSTLEN, "%d.%d.%d.%d", h1, h2, h3, h4);
    
    // Calculer le port (p1 * 256 + p2)
    *client_port = p1 * 256 + p2;
    
    return 0;
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

     // Ignorer SIGCHLD pour éviter les processus zombies
     signal(SIGCHLD, SIG_IGN);

     // Boucle principale pour accepter plusieurs clients
     while (1) {
         len = sizeof(struct sockaddr_storage);
         // Attente connexion du client
         // Lorsque demande de connexion, creation d'une socket de communication avec le client
         descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
         if (descSockCOM == -1){
             perror("Erreur accept\n");
             continue; // Continuer à écouter même en cas d'erreur
         }

         // Fork pour gérer le client dans un processus fils
         pid_t pid = fork();
         if (pid == -1) {
             perror("Erreur fork");
             close(descSockCOM);
             continue;
         }

         if (pid > 0) {
             // Processus PARENT : ferme la socket client et continue d'écouter
             close(descSockCOM);
             continue;
         }

         // Processus FILS : ferme la socket RDV et gère la session client
         close(descSockRDV);

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

        // Variables pour la connexion de données
        char client_data_ip[MAXHOSTLEN];
        int client_data_port = 0;
        char server_data_ip[MAXHOSTLEN];
        int server_data_port = 0;
        int data_transfer_pending = 0; // Flag indiquant qu'un transfert de données est en attente

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
                int n = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
                if (n <= 0) break; // Déconnexion du client
                buffer[n] = '\0'; // Null-terminate pour pouvoir utiliser strncmp
                
                // Interception de la commande PORT
                if (strncmp(buffer, "PORT ", 5) == 0) {
                    printf("PORT intercepté: %s", buffer);
                    
                    // Parser la commande PORT du client
                    if (parse_port_command(buffer, client_data_ip, &client_data_port) == 0) {
                        printf("Client data address: %s:%d\n", client_data_ip, client_data_port);
                        
                        // Envoyer PASV au serveur au lieu de PORT
                        write(descSockServer, "PASV\r\n", 6);
                        printf("Envoi de PASV au serveur\n");
                        data_transfer_pending = 1;
                    } else {
                        // Erreur de parsing, relayer la commande telle quelle
                        write(descSockServer, buffer, n);
                    }
                }
                // Détection des commandes de transfert de données (LIST, RETR, STOR, NLST)
                else if (data_transfer_pending && 
                         (strncmp(buffer, "LIST", 4) == 0 || 
                          strncmp(buffer, "RETR", 4) == 0 || 
                          strncmp(buffer, "STOR", 4) == 0 ||
                          strncmp(buffer, "NLST", 4) == 0)) {
                    
                    printf("Commande de transfert détectée: %s", buffer);
                    
                    // 1. Se connecter au serveur en mode passif
                    int descSockDataServer;
                    char server_port_str[16];
                    snprintf(server_port_str, sizeof(server_port_str), "%d", server_data_port);
                    
                    if (connect2Server(server_data_ip, server_port_str, &descSockDataServer) == -1) {
                        perror("Erreur connexion data au serveur");
                        char *err = "425 Can't open data connection.\r\n";
                        write(descSockCOM, err, strlen(err));
                    } else {
                        printf("Connecté au serveur data: %s:%d\n", server_data_ip, server_data_port);
                        
                        // 2. Se connecter au client en mode actif
                        int descSockDataClient;
                        char client_port_str[16];
                        snprintf(client_port_str, sizeof(client_port_str), "%d", client_data_port);
                        
                        if (connect2Server(client_data_ip, client_port_str, &descSockDataClient) == -1) {
                            perror("Erreur connexion data au client");
                            close(descSockDataServer);
                            char *err = "425 Can't open data connection to client.\r\n";
                            write(descSockCOM, err, strlen(err));
                        } else {
                            printf("Connecté au client data: %s:%d\n", client_data_ip, client_data_port);
                            
                            // Envoyer la commande au serveur
                            write(descSockServer, buffer, n);
                            
                            // 3. Relayer les données entre client et serveur
                            fd_set data_rset;
                            int data_maxfd = (descSockDataServer > descSockDataClient ? descSockDataServer : descSockDataClient) + 1;
                            char data_buffer[MAXBUFFERLEN];
                            struct timeval timeout;
                            
                            while (1) {
                                FD_ZERO(&data_rset);
                                FD_SET(descSockDataServer, &data_rset);
                                FD_SET(descSockDataClient, &data_rset);
                                
                                timeout.tv_sec = 1;
                                timeout.tv_usec = 0;
                                
                                int sel = select(data_maxfd, &data_rset, NULL, NULL, &timeout);
                                if (sel < 0) {
                                    perror("select data error");
                                    break;
                                }
                                if (sel == 0) {
                                    // Timeout - pas de données, fin du transfert
                                    printf("Data transfer timeout (normal end)\n");
                                    break;
                                }
                                
                                // Serveur -> Client (pour LIST/RETR)
                                if (FD_ISSET(descSockDataServer, &data_rset)) {
                                    int dn = read(descSockDataServer, data_buffer, MAXBUFFERLEN);
                                    if (dn <= 0) {
                                        printf("Fin données serveur\n");
                                        break;
                                    }
                                    write(descSockDataClient, data_buffer, dn);
                                }
                                
                                // Client -> Serveur (pour STOR)
                                if (FD_ISSET(descSockDataClient, &data_rset)) {
                                    int dn = read(descSockDataClient, data_buffer, MAXBUFFERLEN);
                                    if (dn <= 0) {
                                        printf("Fin données client\n");
                                        break;
                                    }
                                    write(descSockDataServer, data_buffer, dn);
                                }
                            }
                            
                            close(descSockDataClient);
                            printf("Socket data client fermée\n");
                        }
                        close(descSockDataServer);
                        printf("Socket data serveur fermée\n");
                    }
                    
                    data_transfer_pending = 0; // Reset pour le prochain transfert
                } else {
                    // Relayer normalement les autres commandes
                    write(descSockServer, buffer, n);
                }
            }

            // Serveur -> Proxy -> Client
            if (FD_ISSET(descSockServer, &rset)) {
                int n = read(descSockServer, buffer, MAXBUFFERLEN - 1);
                if (n <= 0) break; // Déconnexion du serveur
                buffer[n] = '\0';
                
                // Interception de la réponse 227 (Entering Passive Mode)
                if (data_transfer_pending && strncmp(buffer, "227 ", 4) == 0) {
                    printf("Réponse PASV reçue: %s", buffer);
                    
                    // Parser la réponse 227 pour extraire l'IP et le port du serveur
                    int h1, h2, h3, h4, p1, p2;
                    char *start = strchr(buffer, '(');
                    if (start && sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
                        snprintf(server_data_ip, MAXHOSTLEN, "%d.%d.%d.%d", h1, h2, h3, h4);
                        server_data_port = p1 * 256 + p2;
                        printf("Server data address: %s:%d\n", server_data_ip, server_data_port);
                        
                        // Envoyer une fausse réponse "200 PORT command successful" au client
                        // Car le client pense qu'il est en mode actif
                        char *fake_response = "200 PORT command successful.\r\n";
                        write(descSockCOM, fake_response, strlen(fake_response));
                        printf("Envoi de fausse réponse PORT au client\n");
                    } else {
                        // Erreur de parsing, relayer tel quel
                        write(descSockCOM, buffer, n);
                    }
                } else {
                    // Relayer normalement les autres réponses
                    write(descSockCOM, buffer, n);
                }
            }
        }
        
        close(descSockServer);
        close(descSockCOM);
        printf("Session closed.\n");
        exit(0); // Fin du processus fils

    } else {
        char *err = "500 Syntax error, command unrecognized or missing @host\r\n";
        write(descSockCOM, err, strlen(err));
        close(descSockCOM);
        exit(0); // Fin du processus fils même en cas d'erreur
    }

    } // Fin de la boucle while (jamais atteinte par le parent)

    close(descSockRDV);
    return 0;
}

