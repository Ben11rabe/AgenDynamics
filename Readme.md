# Introduction et Contexte

## Objectif du projet
Le projet vise à créer un **module d’affichage pour salles et laboratoires à l’ESEO** qui soit :  
- **Facile à lire**, même en plein soleil.  
- **Sécurisé**, avec un accès réservé aux administrateurs pour la maintenance via un lecteur RFID.  
- **Peu coûteux**, avec un coût de production inférieur à 30 € par module.  
- **Évolutif et autonome**, capable de fournir des informations en temps réel.

Chaque module est conçu pour être installé devant une salle ou un laboratoire afin d’afficher instantanément l’emploi du temps et l’occupation de la salle. Le module intègre un **capteur RFID** pour permettre aux utilisateurs autorisés (personnel de maintenance) d’accéder à un mode administrateur et de sélectionner la salle afin de mettre à jour ou gérer l’affichage.

## Problématique
À l’ESEO, l’accès aux salles de classe et aux laboratoires se fait via les badges des étudiants et enseignants, et les emplois du temps sont disponibles uniquement en ligne. Il est donc **impossible de savoir instantanément si une salle est libre ou occupée depuis les couloirs**.  

**Problématique principale** :  
> Comment créer un module d’affichage facile à lire, sécurisé, peu coûteux et évolutif pour les salles, qui fournisse des informations autonomes en temps réel à tous les utilisateurs de l’ESEO ?

## Motivation et contexte
L’objectif est de rendre l’information sur l’occupation des salles **visible directement à côté de chaque porte** grâce à un écran connecté à ESEO-NET. Le projet s’inscrit dans une volonté de moderniser l’affichage des informations internes à l’école, tout en garantissant un coût faible, une consommation énergétique maîtrisée et une installation simple.

## Objectifs spécifiques
Dans l’ordre de priorité :  
1. Lisibilité optimale, même en plein soleil.  
2. Affichage en temps réel des informations.  
3. Faible coût de production (via un PCB optimisé).  
4. Utilisation améliorée pour les administrateurs et les utilisateurs.  
5. Sécurité renforcée : seul le personnel autorisé peut accéder au mode maintenance grâce à la carte RFID.

## Public cible
- **Utilisateurs finaux** : étudiants, enseignants et administration de l’ESEO.  
- **Installation et maintenance** : personnel de maintenance et administrateurs.

## Contraintes et améliorations par rapport à l’état de l’art
- **Coût** : < 30 € par module (lecteur NFC + ESP32 + boutons + écran).  
- **Affichage et mises à jour** : informations en temps réel avec mises à jour toutes les heures (XML → JSON).  
- **Consommation électrique** : conception à faible consommation d’énergie, courant de pointe < 100 mA sur 24 V.  
- **Sécurité** : accès réservé aux administrateurs via carte NFC.  
- **Installation** : facile à installer, retirer et remplacer.  

**Améliorations par rapport à la solution existante** :  
- Écran E-ink 6 pouces avec interface parallèle.  
- Réduction des coûts de 34 %.  
- Changements significatifs dans la mise en œuvre.  
- Convertisseur 24 V CC/CC amélioré.



