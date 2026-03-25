Voici la traduction complète du README.md en français, prête à être copiée-collée dans votre dépôt GitHub pour le rendu à M. CARDAILLAC :



Polytech OS User - TP2 : Protocole BEUIP

Auteurs

MA Zhaoyi



————————————————————————Etape 1,2—————————————————————————————

Présentation du Projet

Ce projet est la réalisation du TP2 du cours "Système d'Exploitation (OS User)" de Polytech Sorbonne.

Nous avons implémenté le protocole BEUIP (BEUI over IP), un protocole de communication en réseau local simplifié et inspiré de NetBEUI. Ce dépôt contient l'implémentation complète de l'Étape 1 (Datagrammes avec accusé de réception) et de l'Étape 2 (Découverte des utilisateurs et routage des messages sur le réseau local).



Afin de respecter scrupuleusement les consignes de qualité du code, l'ensemble du projet compile sans aucune erreur ni avertissement grâce aux options -Wall -Werror. 

————————————————————————————————————————————————————————

Structure du Code

Le code est divisé en deux grandes étapes :



Étape 1 : Datagrammes avec Accusé de Réception

servudp.c : Serveur UDP de base. À la réception d'un message, il utilise l'option MSG\_CONFIRM de la fonction sendto() pour renvoyer automatiquement un accusé de réception ("Bien reçu 5/5 !") au client.



cliudp.c : Client UDP de base. Après l'envoi d'un message, il fait un appel bloquant sur recvfrom() pour attendre et afficher l'accusé de réception envoyé par le serveur.



Étape 2 : Protocole BEUIP v1

servbeuip.c : Le serveur principal du protocole BEUIP, attaché au port 9998. Ses rôles sont :



Envoyer un message de broadcast sur 192.168.88.255 lors de son démarrage.



Maintenir une table des utilisateurs actifs sur le réseau local (contenant l'adresse IP et le pseudo, avec une capacité maximale de 255 entrées).



Écouter les datagrammes BEUIP, mettre à jour la table, transférer les messages et gérer les accusés de réception.



clibeuip.c : L'interface de contrôle locale (client). Il envoie uniquement des commandes de contrôle (demande de liste, envoi de message, etc.) au serveur local sur 127.0.0.1

——————————————————————————————————————————————————————

Fonctionnalités Réalisées

Conformément au protocole, nous avons implémenté le traitement des codes de commande suivants :



Code 1 (Broadcast) \& Code 2 (AR) :

Au démarrage, le serveur diffuse son Pseudo en broadcast (Code 1). Les autres servbeuip le reçoivent, l'ajoutent à leur table, et répondent avec un accusé de réception contenant leur propre Pseudo (Code 2).



Code 3 (Liste des utilisateurs) :

Lorsque clibeuip envoie cette commande, le serveur affiche sur son terminal la liste de tous les utilisateurs (Pseudo et IP) actuellement enregistrés dans sa table. Une vérification de sécurité est appliquée : cette commande n'est acceptée que si elle provient de 127.0.0.1.



Code 4 -> 9 (Message Privé) :

clibeuip envoie un datagramme au serveur local contenant le Pseudo cible et le message, séparés par un \\0. Le serveur cherche l'adresse IP correspondante dans sa table et transfère le message à la cible en le convertissant au format Code 9.



Code 5 -> 9 (Message à tous) :

Fonction de diffusion. Le serveur parcourt sa table d'utilisateurs et envoie le message (sous forme de Code 9) à toutes les adresses IP enregistrées, sauf la sienne.



Code 0 (Déconnexion) :

Lors de l'arrêt, un broadcast est envoyé pour informer les autres nœuds du réseau de son départ. Les serveurs qui le reçoivent suppriment ce couple (pseudo + IP) de leur table locale.

——————————————————————————————————————————————————————————Compilation et Exécution

Compilation

Un Makefile est fourni.

La compilation se fait avec les options strictes (-Wall -Werror) ainsi que la macro de traçage (-DTRACE) :



make clean

make

Exécution (Tests Étape 2)

Lancement du serveur (Terminal 1) :

Il est nécessaire de fournir un pseudo en paramètre.



./servbeuip MonPseudo

Envoi de commandes de contrôle (Terminal 2) :

Utilisez le client local pour interagir avec le serveur.



./clibeuip 3                      # Afficher la liste des utilisateurs en ligne

./clibeuip 4 Destinataire "Salut" # Envoyer un message privé

./clibeuip 5 "Bonjour à tous"     # Diffuser un message à tous

./clibeuip 0                      # Quitter le réseau proprement





—————————————————————————————Etape 3——————————————————————————



&#x20;Structure du Code (Mise à jour)

creme.c / creme.h : Création de la bibliothèque creme (Commandes Rapides pour l'Envoi de Messages Evolués). Elle encapsule toute la logique du serveur BEUIP et l'envoi des commandes locales en UDP. Les symboles exportés ont été rigoureusement vérifiés avec la commande nm.



gescom.c / gescom.h : Mise à jour du gestionnaire de commandes internes pour inclure les appels à la bibliothèque creme.



biceps.c : Le programme principal reste inchangé grâce à l'architecture modulaire de gescom.

——————————————————————————————————————————————————————————

Nouvelles Commandes Internes

Notre shell biceps possède désormais des commandes réseaux natives :



beuip start <pseudo> : Crée un processus fils (via fork()) qui exécute en arrière-plan la boucle du serveur BEUIP. Le PID du fils est sauvegardé par le processus parent.



beuip stop : Envoie un signal (SIGTERM) au processus fils serveur. Le serveur intercepte ce signal, diffuse un message de déconnexion (Code 0) en broadcast, puis s'arrête proprement.



mess liste : Demande et affiche la liste des utilisateurs présents sur le réseau.



mess a <pseudo> <msg> : Envoie un message privé à l'utilisateur ciblé. (Note : le "à" a été remplacé par "a" pour éviter les problèmes d'encodage des caractères accentués dans le shell).



mess tous <msg> : Diffuse un message à l'ensemble des utilisateurs connectés sur le réseau local.

——————————————————————————————————————————————————————————

Compilation et Exécution (Étape 3)

Compilation

La compilation du projet final (qui intègre le shell biceps, le gestionnaire de commandes gescom et la nouvelle bibliothèque réseau creme) est entièrement automatisée via le Makefile.



Pour respecter les exigences d'évaluation, la compilation est configurée de manière très stricte avec les drapeaux -Wall et -Werror (zéro avertissement toléré). L'option -DTRACE est également incluse pour activer les affichages de débogage réseau lors de l'exécution.



\# Nettoyer les anciens fichiers objets et l'exécutable

make clean



\# Compiler le projet complet

make



Exécution

Une fois la compilation réussie, lancez votre interpréteur de commandes interactif biceps :



./biceps

Désormais, depuis l'invite de commande de votre propre shell, vous pouvez utiliser les nouvelles commandes internes de gestion du réseau :



Démarrer votre serveur réseau en arrière-plan :



biceps$ beuip start MonPseudo



Communiquer avec les autres étudiants :



biceps$ mess liste                  # Consulter la liste des utilisateurs en ligne

biceps$ mess a Alice Bonjour!       # Envoyer un message privé à l'utilisateur "Alice"

biceps$ mess tous Salut a tous !    # Diffuser un message à toutes les personnes connectées



Quitter proprement le réseau :

biceps$ beuip stop

