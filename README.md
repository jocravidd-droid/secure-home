# 🛡️ Secure Home — Alarme domestique ESP32-CAM + Telegram

<p align="center">
  <img src="logo.png" alt="Secure Home Agency" width="160">
</p>

<p align="center">
  <strong>Surveillance & Alerte Intrusion — Notification instantanée en temps réel via ESP32-CAM</strong>
</p>

---

## 📋 Description

**Secure Home** est un système d'alarme domestique fait maison, basé sur un module **ESP32-CAM**. Il surveille une porte ou un passage (escalier) et envoie une alerte instantanée avec photo directement sur Telegram dès qu'une intrusion est détectée. Le système peut aussi être contrôlé et interrogé à distance via un bot Telegram dédié (`@secureh_bot`).

Projet à usage strictement personnel — non destiné à la revente ou à la distribution commerciale.

## ✨ Fonctionnalités

- 🚪 **Détection double mode** : capteur magnétique de porte (MC-38) ou barrière laser (tripwire), au choix via un interrupteur physique
- 📸 **Photo automatique** envoyée sur Telegram à chaque intrusion détectée
- 🔔 **Buzzer sonore** local déclenché en simultané
- 🔒 **Armement / désarmement à distance** (`/arme`, `/desarme`)
- 📷 **Photo à la demande** sans déclencher l'alarme (`/photo`)
- 🔕 **Mode silence temporaire** (`/silence 10`)
- 🔄 **Changement de mode à distance** (`/mode laser` / `/mode porte`)
- 🕓 **Historique** des 5 derniers événements (`/historique`)
- ✅ **Bouton "J'ai vu"** intégré sous chaque alerte photo pour couper le buzzer à distance
- 📊 **Statut en temps réel** (armé/désarmé, mode actif, signal Wi-Fi) via `/statut`

## 🧰 Matériel utilisé

| Composant | Rôle |
|---|---|
| ESP32-CAM (AI-Thinker, OV2640/OV3660) + module MB | Cerveau du système + caméra |
| Capteur de porte magnétique MC-38 (NC/NO) | Détection d'ouverture de porte |
| Module laser émetteur KY-008 + récepteur | Détection de type barrière laser |
| Buzzer actif 5V | Sirène locale |
| Interrupteur à bascule (aviation) | Sélecteur de mode LASER / PORTE |
| Breadboard MB-102 + fils Dupont | Câblage sans soudure |
| Batterie externe (power bank) 10 000 mAh | Alimentation autonome et sécurisée |

## 🔌 Câblage (broches ESP32-CAM)

| Broche | Composant |
|---|---|
| GPIO 12 | Capteur de porte MC-38 |
| GPIO 13 | Buzzer |
| GPIO 14 | Laser (émetteur) |
| GPIO 2 | Récepteur laser |
| GPIO 15 | Interrupteur de mode (LASER/PORTE) |

⚠️ GPIO 12 est une broche de *strapping* : la porte doit être **fermée** au moment du démarrage/reset de la carte.

## 🤖 Commandes du bot Telegram

```
/statut     - État du système (armé, mode, signal Wi-Fi)
/arme       - Armer l'alarme à distance
/desarme    - Désarmer l'alarme à distance
/photo      - Prendre une photo à la demande
/silence 10 - Silence temporaire (en minutes)
/historique - Voir les 5 dernières alertes
/mode laser - Basculer en mode laser
/mode porte - Basculer en mode porte
/aide       - Afficher l'aide
```

## ⚙️ Installation

1. Installer [Arduino IDE](https://www.arduino.cc/en/software) (2.x recommandé)
2. Ajouter le support ESP32 via **Fichier > Préférences > URL de gestionnaire de cartes supplémentaires** :
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Installer la carte **esp32 by Espressif Systems** (Outils > Carte > Gestionnaire de cartes)
4. Installer la librairie **ArduinoJson** (Outils > Gérer les bibliothèques)
5. Sélectionner la carte **AI Thinker ESP32-CAM**
6. Renseigner dans le code : `ssid`, `password`, `token` (BotFather), `chat_id`
7. Téléverser le programme sur le module

## 🔐 Sécurité & confidentialité

- Le fichier `.ino` contenant les identifiants Wi-Fi et le token du bot **n'est jamais versionné** sur ce dépôt
- Toute commande Telegram est vérifiée par comparaison avec le `chat_id` autorisé ; les autres sont ignorées
- Voir la [politique de confidentialité complète](./privacy.html)

## 📄 Licence

Projet personnel — tous droits réservés à l'auteur. Non destiné à la redistribution.

---

<p align="center"><em>Secure Home Agency — Est. 2026 — Bourail Div.</em></p>
