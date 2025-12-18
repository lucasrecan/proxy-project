# Mini projet Proxy FTP (en binômes)

Pour l'instant on a le socket de connexion qui fonctionne, avec une connexion au client et une connexion au serveur, et maintenant il faut ouvrir un second socket qui permet d'échanger des données (voir fiche-ftp.pdf), avec une connexion avec le client et une connexion avec le serveur. 

Pour se connecter, on execute ./proxy dans un terminal et ftp -z nossl -d <ip-serveur> <num-port> dans un autre terminal.

Dans notre code il manque la récupération du mot de passe par le serveur, que le proxy doit prendre en charge.

Le proxy doit pouvoir suppporter plusieurs clients : pour chaque nouveau client on fait un fork().