# Mini projet Proxy FTP (en binômes)

Ce projet consiste en la réalisation d'un proxy FTP intermédiaire capable de traduire les modes de connexion entre un client et un serveur.

L'enjeu technique principal est de permettre à un client FTP ancien (type DOS), limité au mode actif, de communiquer avec un serveur distant via le mode passif, tout en gérant les contraintes de signalisation réseau.

## Fonctionnalités clés

- Translation de mode : Supporte le mode Actif côté client et le convertit en mode Passif côté serveur.
- Relai de Contrôle : Gestion des commandes et codes de retour FTP via une socket de contrôle.
- Relai de Données : Création dynamique de sockets pour le transfert de fichiers et le listing (ls).
- Multi-sessions : Capacité à gérer plusieurs connexions simultanées (via fork ou threads/select selon ton implémentation).
- Authentification transparente : Support de la syntaxe user@host pour l'aiguillage automatique vers le serveur cible.
