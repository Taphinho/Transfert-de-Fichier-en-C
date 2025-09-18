# 📂 Projet : Transfert de fichiers (TCP)

## 📝 Description
Ce projet consiste à développer une application **client-serveur** permettant le **transfert fiable de fichiers** en utilisant le protocole **TCP**.

- **Côté serveur** : l’application accepte les connexions des clients et gère deux types d’opérations :
  - **GET** : téléchargement d’un fichier existant depuis le serveur.
  - **WRITE** : téléversement d’un fichier vers le serveur.

- **Côté client** : l’utilisateur peut envoyer une requête au serveur en respectant la syntaxe suivante :
  - `GET:filename` → demande de téléchargement.
  - `WRITE:filename:size:number` → demande de téléversement, où :
    - **size** : taille totale du fichier en octets.
    - **number** : nombre de paquets nécessaires à l’envoi.

L’échange repose sur une **communication par paquets** garantissant l’intégrité et la complétude des données grâce au protocole **TCP**.

---

## 🔄 Réponses du serveur
- À une requête **GET** :
  - `OK:size:number` puis envoi des paquets constituant le fichier découpé.
  - ou message d’erreur si le fichier n’existe pas.
- À une requête **WRITE** :
  - `READY` → le serveur est prêt à recevoir les paquets.
  - Réception et stockage du fichier transmis.

---

## ⚙️ Aspects techniques
- Manipulation des fichiers avec **fread()** et **fwrite()**.  
- Gestion dynamique de la mémoire avec **calloc()**.  
- Utilisation de **pointeurs** et de **memcpy()** pour gérer les buffers de données.  
- Boucles `for` pour envoyer et recevoir séquentiellement les paquets.  
- Les adresses (IP) et le port du serveur sont prédéfinis et connus du client.  

---

## 📡 Schéma de communication
1. Connexion du client au serveur (TCP).  
2. Envoi d’une requête (**GET** ou **WRITE**).  
3. Réponse du serveur (`OK:size:number` ou `READY`).  
4. Transfert des paquets (envoi ou réception selon le cas).  
5. Reconstruction et stockage du fichier complet.  
