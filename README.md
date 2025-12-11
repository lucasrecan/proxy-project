# Mini projet Proxy FTP (en binômes)

# Objectif

- Permettre à un client FTP en mode actif (DOS) de fonctionner contre un serveur FTP distant en mode passif, via un proxy.
- Le proxy se comporte comme un serveur FTP vis-à-vis du client, et comme un client FTP vis-à-vis du serveur.
- L’utilisateur s’authentifie avec USER nomlogin@nomserveur . Le proxy crée la connexion de contrôle vers nomserveur , envoie USER nomlogin au vrai serveur, puis relaie toutes les commandes/réponses.
- Pour la commande ls (FTP LIST ), le proxy intercepte les connexions de données: il accepte le côté client en mode actif (PORT) mais bascule en mode passif vers le serveur (PASV), et pont les deux flux.
- Le proxy doit gérer plusieurs sessions simultanées.
Actif vs Passif

- Mode actif (client→serveur): le client envoie PORT h1,h2,h3,h4,p1,p2 , écoute sur ce port, et le serveur se connecte vers le client pour le transfert de données.
- Mode passif (client→serveur): le client envoie PASV , le serveur répond 227 (...) avec une IP/port, et le client se connecte à ce port pour le transfert.
- Ton proxy doit accepter le flux actif côté client et, côté serveur, utiliser PASV , puis faire le pont entre les deux sockets de données.
Fichiers fournis

- src/ProxyFTP/proxy.c : squelette serveur TCP côté proxy à compléter. Il écoute et accepte un client, affiche l’adresse/port, puis envoie une chaîne (voir src/ProxyFTP/proxy.c:64–80 ).
- src/ProxyFTP/simpleSocketAPI.c : utilitaire connect2Server(serverName, port, &descSock) qui gère getaddrinfo+connect (voir src/ProxyFTP/simpleSocketAPI.c:8–59 ).
- src/client.c et src/serveur.c : exemples TCP de base pour comprendre écoute/accept/connect et lire/écrire (voir src/serveur.c:39–99 , src/client.c:41–79 ).
- src/ProxyFTP/Makefile : compilation du proxy et de l’API socket.
Partie 1 — Connexions de contrôle

- Écoute côté proxy sur un port choisi dynamiquement (déjà prêt). Afficher l’IP/port et accepter des clients.
- Dès qu’un client se connecte, envoyer 220 Proxy ready\r\n pour mimer la bannière FTP.
- Lire la ligne USER nomlogin@nomserveur\r\n du client.
  - Extraire nomserveur et nomlogin en séparant sur @ .
  - Établir la connexion de contrôle vers nomserveur:21 avec connect2Server .
  - Lire la bannière 220 du serveur réel mais ne pas la relayer (le client a déjà eu le 220 du proxy).
  - Envoyer USER nomlogin\r\n au serveur réel, puis relayer les réponses 331 , etc., et relayer ensuite toutes les commandes/réponses suivantes de façon transparente.
- Multiplexage contrôle:
  - Boucle qui surveille les deux sockets (client↔proxy et proxy↔serveur) avec select() ou poll() .
  - Toute commande venant du client est envoyée au serveur (sauf celles que tu dois intercepter, voir Partie 2).
  - Toute réponse venant du serveur est renvoyée au client.
- Multisession:
  - Après listen , remplacer le accept unique par une boucle while (1) .
  - À chaque accept , créer une session dédiée ( fork() ou thread) et gérer son contrôle/données dedans.
  - Fermer les descripteurs correctement dans parent/enfant.
Partie 2 — Connexions de données (LIST)

- Intercepter la commande PORT envoyée par le client:
  - Ne pas la relayer au serveur.
  - Parser h1,h2,h3,h4,p1,p2 pour obtenir client_ip et client_port = p1*256 + p2 .
  - Mémoriser cette info pour la prochaine commande de transfert ( LIST ).
- Lorsqu’une commande de transfert arrive (au minimum LIST pour ce projet):
  - Envoyer PASV\r\n au serveur réel et lire la réponse 227 Entering Passive Mode (a,b,c,d,e,f) .
  - Parser l’IP/port du serveur passif: server_ip = a.b.c.d , server_port = e*256 + f .
  - Créer deux sockets de données:
    - Côté serveur: connect(server_ip, server_port) .
    - Côté client: connect(client_ip, client_port) pour simuler la connexion active que le serveur aurait faite vers le client. Le client attend une connexion entrante: ton proxy prend le rôle du serveur et s’y connecte.
  - Envoyer LIST\r\n au serveur réel sur la connexion de contrôle.
  - Pont des données:
    - Lire du socket de données serveur et écrire vers le socket de données client jusqu’à EOF.
    - Optionnellement relayer aussi dans l’autre sens si nécessaire (pour RETR / STOR selon extension).
  - Relayer les réponses de contrôle 150 et 226 au client.
  - Fermer proprement les deux sockets de données.
- État et robustesse:
  - Séparer traitement contrôle et données; déclencher la séquence PASV/Data uniquement quand un transfert est demandé.
  - Gérer timeouts/erreurs: si PASV échoue ou si la connexion client data est impossible, renvoyer un code FTP clair ( 425 , 426 , 451 ).
Points techniques clés

- Protocole FTP en texte avec terminaison CRLF: envoyer/recevoir les lignes avec \r\n .
- Lecture de lignes:
  - Implémenter une fonction de lecture de ligne qui accumule jusqu’à CRLF. Éviter read direct sans gestion de découpage.
- Parsing USER :
  - char *at = strchr(userArg, '@'); pour séparer login et host.
- Parsing 227 :
  - Extraire les nombres entre parenthèses; calculer le port avec e*256 + f .
- PORT du client:
  - Vérifier que l’IP mentionnée est routable depuis le proxy; pour des tests locaux, ce sera généralement 127.0.0.1 ou l’IP de la machine où tourne le client DOS via le proxy.
- Écoute réseau:
  - Actuellement SERVADDR vaut 127.0.0.1 dans src/ProxyFTP/proxy.c:11 . Pour exposer sur toutes interfaces, utiliser une adresse vide ( NULL ) avec AI_PASSIVE ou 0.0.0.0 . En TP, 127.0.0.1 peut suffire pour tests locaux.
- Multisession:
  - LISTENLEN est 1 ( src/ProxyFTP/proxy.c:13 ); augmenter si vous attendez plusieurs connexions simultanées, mais le plus important est le fork() /thread par session.
- Nettoyage:
  - Toujours fermer descSockCOM et les sockets de données en fin de session.
Ce que tu dois coder dans proxy.c

- Bannières et handshake initial côté client ( 220 Proxy ready ).
- Parsing USER login@server , connexion au serveur réel, envoi USER login .
- Boucle de relais de commandes/réponses avec select() .
- Interception PORT côté client: parse et mémorise.
- Lors de LIST : séquence PASV côté serveur, connect data côté serveur et côté client, pont des données, fermeture.
- Concurrence: boucle d’acception et fork() par client.
Références dans le code

- Connexion au serveur: connect2Server déjà prête dans src/ProxyFTP/simpleSocketAPI.c:8–59 .
- Création socket écoute côté proxy: voir src/ProxyFTP/proxy.c:34–63 .
- Affichage IP/port d’écoute: voir src/ProxyFTP/proxy.c:64–80 .
- Exemple d’ accept / write : src/serveur.c:85–99 . Exemple de connect / read : src/client.c:52–79 .

Prochaines étapes

- Implémenter la Partie 1 dans proxy.c : handshake, USER avec parsing, relais contrôle, fork() par session.
- Ajouter l’interception PORT et la séquence PASV pour LIST , avec pont de données.
- Ajouter lecture/écriture ligne par ligne (CRLF) et parser 227 / PORT .
- Tester d’abord localement avec un serveur FTP local, puis avec un serveur externe.