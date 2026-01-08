# Mini projet Proxy FTP (en binômes)

Pour l'instant on a le socket de connexion qui fonctionne, avec une connexion au client et une connexion au serveur, et maintenant il faut ouvrir un second socket qui permet d'échanger des données (voir fiche-ftp.pdf), avec une connexion avec le client et une connexion avec le serveur. 

Pour se connecter, on execute ./proxy dans un terminal et ftp -z nossl -d <ip-serveur> <num-port> dans un autre terminal.

Dans notre code il manque la récupération du mot de passe par le serveur, que le proxy doit prendre en charge.

Le proxy doit pouvoir suppporter plusieurs clients : pour chaque nouveau client on fait un fork().

le proxy doit pas se fermer en cas d'erreur sur un client

Le proxy doit gérer les switch en mode passif/actif

Ce qu'il reste à faire sur le Proxy FTP
D'après le README et les consignes du TP, voici les étapes manquantes pour finaliser le projet :

1. Support Multi-Clients (fork)
Le proxy actuel ne traite qu'un seul client puis s'arrête.

Objectif : Il faut entourer l'appel accept() d'une boucle infinie et utiliser fork() pour chaque nouvelle connexion.
Action : Le processus parent doit continuer d'écouter, tandis que le processus fils gère la session FTP (l'échange de commandes).
2. Interception des Commandes de Contrôle
Pour l'instant, le proxy relaie les messages de manière "aveugle" via select().

Objectif : Il faut analyser les chaînes de caractères (les commandes) envoyées par le client pour repérer quand une commande de transfert de données est envoyée (surtout la commande PORT).
3. Traduction Actif / Passif (Cœur du TP)
C'est la partie la plus technique. Le client utilise le mode Actif, mais le serveur doit être contacté en mode Passif. Voici la logique à coder quand l'utilisateur tape ls :

Côté Client : Intercepter la commande PORT h1,h2,h3,h4,p1,p2 envoyée par le client. (Elle contient l'IP et le port où le client attend les données).
Côté Serveur : Ne pas envoyer PORT au serveur. À la place, le proxy doit envoyer la commande PASV.
Réponse du Serveur : Intercepter la réponse 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
Pont de Données :
Le proxy doit ouvrir une socket et se connecter au serveur (via l'IP/Port reçu dans le message 227).
Le proxy doit ouvrir une autre socket et se connecter au client (via l'IP/Port reçu dans la commande PORT initiale).
Le proxy doit ensuite relayer les données (le résultat du ls) entre ces deux nouvelles sockets.
4. Gestion du Mot de Passe
Le README indique que le proxy doit prendre en charge la récupération du mot de passe.

Action : S'assurer qu'après la commande USER, on gère correctement la réponse 331 Password required du serveur et qu'on transmet bien la commande PASS qui suit.
En résumé, le prochain gros morceau, c'est de sortir de la boucle de relais simple pour détecter la commande PORT et mettre en place le double pont de données.