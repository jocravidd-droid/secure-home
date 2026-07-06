# Secure Home — Alarme domestique ESP32-CAM + Telegram

> **Note de conception :** Ce projet est une réalisation hybride. L'intégration matérielle, le câblage et l'assemblage physique des composants sont entièrement réalisés par le concepteur. La partie logicielle (développement en C++ pour la gestion du système et la connectivité du bot Telegram) a été générée par Intelligence Artificielle afin de pallier la non-maîtrise de ce langage.

<p align="center">
  <img src="logo.png" alt="Secure Home Agency" width="160">
</p>

<p align="center">
  <strong>Surveillance & Alerte Intrusion — Notification instantanée en temps réel via ESP32-CAM</strong>
</p>

---

## ⚠️ NOTICE IMPORTANTE : À LIRE AVANT TOUTE MANIPULATION

* **Nature du code :** Le programme fourni sert uniquement de support fonctionnel pour interconnecter les modules électroniques et l'API Telegram. Il est optimisé pour l'usage matériel décrit ci-dessous.
* **Contrainte matérielle critique :** Le GPIO 12 de l'ESP32-CAM est une broche de *strapping*. La porte (capteur MC-38) doit impérativement être **fermée** lors du démarrage ou du reset électrique de la carte, sous peine de bloquer le démarrage du module.
* **Alimentation :** L'utilisation d'une batterie externe (power bank) de qualité est requise pour assurer la stabilité de la tension lors de l'activation du flash et de l'envoi des images par Wi-Fi.

---

## Description

**Secure Home** est un système d'alarme domestique résidentiel basé sur un module **ESP32-CAM**. Il surveille une porte ou un passage (escalier) et envoie une alerte instantanée avec photo directement sur Telegram dès qu'une intrusion est détectée. Le système peut également être contrôlé et interrogé à distance via un bot Telegram dédié (`@secureh_bot`).

Projet à usage strictement personnel — non destiné à la revente ou à la distribution commerciale.

## Fonctionnalités

- **Détection double mode** : capteur magnétique de porte (MC-38) ou barrière laser (tripwire), au choix via un interrupteur physique.
- **Photo automatique** envoyée sur Telegram à chaque intrusion détectée.
- **Buzzer sonore** local déclenché en simultané.
- **Armement / désarmement à distance** (`/arme`, `/desarme`).
- **Photo à la demande** sans déclencher l'alarme (`/photo`).
- **Mode silence temporaire** (`/silence 10`).
- **Changement de mode à distance** (`/mode laser` / `/mode porte`).
- **Historique** des 5 derniers événements (`/historique`).
- **Bouton "J'ai vu"** intégré sous chaque alerte photo pour couper le buzzer à distance.
- **Statut en temps réel** (armé/désarmé, mode actif, signal Wi-Fi) via `/statut`.

## Matériel utilisé

| Composant | Rôle |
|---|---|
| ESP32-CAM (AI-Thinker, OV2640/OV3660) + module MB | Cerveau du système + caméra |
| Capteur de porte magnétique MC-38 (NC/NO) | Détection d'ouverture de porte |
| Module laser émetteur KY-008 + récepteur | Détection de type barrière laser |
| Buzzer actif 5V | Sirène locale |
| Interrupteur à bascule (aviation) | Sélecteur de mode LASER / PORTE |
| Breadboard MB-102 + fils Dupont | Câblage sans soudure |
| Batterie externe (power bank) 10 000 mAh | Alimentation autonome et sécurisée |

## Câblage (broches ESP32-CAM)

| Broche | Composant |
|---|---|
| GPIO 12 | Capteur de porte MC-38 |
| GPIO 13 | Buzzer |
| GPIO 14 | Laser (émetteur) |
| GPIO 2  | Récepteur laser |
| GPIO 15 | Interrupteur de mode (LASER/PORTE) |

## Commandes du bot Telegram

```text
/statut     - État du système (armé, mode, signal Wi-Fi)
/arme       - Armer l'alarme à distance
/desarme    - Désarmer l'alarme à distance
/photo      - Prendre une photo à la demande
/silence 10 - Silence temporaire (en minutes)
/historique - Voir les 5 dernières alertes
/mode laser - Basculer en mode laser
/mode porte - Basculer en mode porte
/aide       - Afficher l'aide
