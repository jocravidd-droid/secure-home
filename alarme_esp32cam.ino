// ====================================================================
//   ALARME ESP32-CAM — VERSION 2 (Contrôle à distance via Telegram)
//   Nécessite la librairie "ArduinoJson" (Benoit Blanchon) installée
//   via Outils > Gérer les bibliothèques dans Arduino IDE
// ====================================================================

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <ArduinoJson.h>

// ====================================================================
// 1. ZONE DE CONFIGURATION (À REMPLIR)
// ====================================================================
const char* ssid     = "TON_WIFI";
const char* password = "TON_MOT_DE_PASSE";

const String token   = "TON_TOKEN_BOT_TELEGRAM";
const String chat_id = "TON_CHAT_ID";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// ====================================================================
// 2. CONFIGURATION DES BROCHES (PINS)
// ====================================================================
const int PIN_CAPTEUR_PORTE   = 12; // Capteur de porte MC-38
// ATTENTION strapping pin : porte doit être FERMÉE au démarrage/reset.
const int PIN_BUZZER          = 13;
const int PIN_LASER_EMETTEUR  = 14;
const int PIN_LASER_RECEPTEUR = 2;  // Sortie digitale du module récepteur laser
const int PIN_MODE_SWITCH     = 15; // Interrupteur rouge : HIGH = mode LASER, LOW = mode PORTE

// Pins caméra (AI-THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ====================================================================
// 3. ÉTAT DU SYSTÈME
// ====================================================================
enum Mode { MODE_LASER, MODE_PORTE };
Mode currentMode = MODE_PORTE;
bool lastSwitchState = LOW;

bool systemArmed = true;
unsigned long silenceUntil = 0;
long lastUpdateId = 0;

const int HIST_SIZE = 5;
String historique[HIST_SIZE];
int histIndex = 0;

unsigned long lastPollTime = 0;
const unsigned long POLL_INTERVAL = 2000;

// ====================================================================
// SETUP
// ====================================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_CAPTEUR_PORTE, INPUT_PULLUP);
  pinMode(PIN_MODE_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LASER_RECEPTEUR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LASER_EMETTEUR, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LASER_EMETTEUR, LOW);

  lastSwitchState = digitalRead(PIN_MODE_SWITCH);
  currentMode = lastSwitchState == HIGH ? MODE_LASER : MODE_PORTE;

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Échec init caméra 0x%x\n", err);
  }

  Serial.print("Connexion Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connecté !");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  ajouterHistorique("Démarrage du système");
  envoyerMessageTexte("✅ Alarme démarrée. Mode actuel : " + String(currentMode == MODE_LASER ? "LASER" : "PORTE"));
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {
  bool switchState = digitalRead(PIN_MODE_SWITCH);
  if (switchState != lastSwitchState) {
    lastSwitchState = switchState;
    currentMode = switchState == HIGH ? MODE_LASER : MODE_PORTE;
    ajouterHistorique("Mode changé (interrupteur physique) : " + String(currentMode == MODE_LASER ? "LASER" : "PORTE"));
    envoyerMessageTexte("🔄 Mode changé via interrupteur : " + String(currentMode == MODE_LASER ? "LASER" : "PORTE"));
  }

  if (millis() - lastPollTime > POLL_INTERVAL) {
    lastPollTime = millis();
    verifierCommandesTelegram();
  }

  bool enSilence = silenceUntil != 0 && millis() < silenceUntil;
  if (!systemArmed || enSilence) {
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LASER_EMETTEUR, LOW);
    delay(50);
    return;
  }

  bool intrusion = false;

  if (currentMode == MODE_LASER) {
    digitalWrite(PIN_LASER_EMETTEUR, HIGH);
    if (digitalRead(PIN_LASER_RECEPTEUR) == LOW) {
      intrusion = true;
    }
  } else {
    if (digitalRead(PIN_CAPTEUR_PORTE) == HIGH) {
      intrusion = true;
    }
  }

  if (intrusion) {
    Serial.println("INTRUSION DÉTECTÉE !");
    ajouterHistorique("Intrusion (" + String(currentMode == MODE_LASER ? "laser" : "porte") + ") - " + getHorodatage());

    digitalWrite(PIN_BUZZER, HIGH);
    capturerEtEnvoyerAlerte();

    delay(15000);
    digitalWrite(PIN_BUZZER, LOW);
  }

  delay(10);
}

// ====================================================================
// TÉLÉCHARGEMENT ET TRAITEMENT DES COMMANDES TELEGRAM
// ====================================================================
void verifierCommandesTelegram() {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + token + "/getUpdates?offset=" + String(lastUpdateId + 1) + "&timeout=0";
  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Connection: close");
  client.println();

  unsigned long t = millis();
  while (client.connected() && millis() - t < 4000) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }
  }

  String body = "";
  while (client.available()) {
    body += client.readStringUntil('\n');
  }
  client.stop();

  if (body.length() < 10) return;

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, body);
  if (error) return;

  JsonArray results = doc["result"].as<JsonArray>();
  for (JsonObject update : results) {
    long updateId = update["update_id"];
    if (updateId > lastUpdateId) lastUpdateId = updateId;

    if (update.containsKey("callback_query")) {
      String data = update["callback_query"]["data"].as<String>();
      String cbId = update["callback_query"]["id"].as<String>();
      if (data == "ack") {
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_LASER_EMETTEUR, LOW);
        repondreCallback(cbId, "Alerte acquittée ✅");
        ajouterHistorique("Alerte acquittée depuis Telegram");
      }
      continue;
    }

    if (!update.containsKey("message")) continue;
    String msgChatId = update["message"]["chat"]["id"].as<String>();
    if (msgChatId != chat_id) continue;

    String text = update["message"]["text"].as<String>();
    traiterCommande(text);
  }
}

void traiterCommande(String text) {
  text.trim();

  if (text == "/statut") {
    String msg = "📊 État du système\n";
    msg += "Armé : " + String(systemArmed ? "Oui ✅" : "Non ❌") + "\n";
    msg += "Mode : " + String(currentMode == MODE_LASER ? "LASER 🔴" : "PORTE 🚪") + "\n";
    msg += "Signal Wi-Fi : " + String(WiFi.RSSI()) + " dBm\n";
    msg += "Heure : " + getHorodatage();
    envoyerMessageTexte(msg);

  } else if (text == "/arme") {
    systemArmed = true;
    ajouterHistorique("Armé à distance");
    envoyerMessageTexte("🔒 Système armé.");

  } else if (text == "/desarme") {
    systemArmed = false;
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LASER_EMETTEUR, LOW);
    ajouterHistorique("Désarmé à distance");
    envoyerMessageTexte("🔓 Système désarmé.");

  } else if (text == "/photo") {
    envoyerMessageTexte("📸 Capture en cours...");
    capturerEtEnvoyerPhotoSimple();

  } else if (text.startsWith("/silence")) {
    int minutes = 5;
    int spaceIndex = text.indexOf(' ');
    if (spaceIndex > 0) {
      minutes = text.substring(spaceIndex + 1).toInt();
      if (minutes <= 0) minutes = 5;
    }
    silenceUntil = millis() + (unsigned long)minutes * 60000UL;
    ajouterHistorique("Silence " + String(minutes) + " min activé");
    envoyerMessageTexte("🔕 Silence activé pendant " + String(minutes) + " minute(s).");

  } else if (text == "/historique") {
    String msg = "🕓 Dernières alertes :\n";
    bool vide = true;
    for (int i = 0; i < HIST_SIZE; i++) {
      int idx = (histIndex + i) % HIST_SIZE;
      if (historique[idx].length() > 0) {
        msg += "- " + historique[idx] + "\n";
        vide = false;
      }
    }
    if (vide) msg = "Aucun événement enregistré depuis le démarrage.";
    envoyerMessageTexte(msg);

  } else if (text == "/mode laser") {
    currentMode = MODE_LASER;
    ajouterHistorique("Mode changé à distance : LASER");
    envoyerMessageTexte("🔴 Mode LASER activé (jusqu'au prochain changement d'interrupteur).");

  } else if (text == "/mode porte") {
    currentMode = MODE_PORTE;
    ajouterHistorique("Mode changé à distance : PORTE");
    envoyerMessageTexte("🚪 Mode PORTE activé (jusqu'au prochain changement d'interrupteur).");

  } else if (text == "/aide" || text == "/start") {
    String msg = "🕵️ Commandes disponibles :\n";
    msg += "/statut - état du système\n";
    msg += "/arme - armer à distance\n";
    msg += "/desarme - désarmer à distance\n";
    msg += "/photo - photo à la demande\n";
    msg += "/silence [min] - silence temporaire\n";
    msg += "/historique - dernières alertes\n";
    msg += "/mode laser ou /mode porte - changer de mode";
    envoyerMessageTexte(msg);
  }
}

void ajouterHistorique(String event) {
  historique[histIndex] = event;
  histIndex = (histIndex + 1) % HIST_SIZE;
}

String getHorodatage() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "Heure indisponible";
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// ====================================================================
// ENVOI DE MESSAGES TEXTE SIMPLES
// ====================================================================
void envoyerMessageTexte(String texte) {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + token + "/sendMessage?chat_id=" + chat_id + "&text=" + urlEncode(texte);
  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Connection: close");
  client.println();
  delay(100);
  client.stop();
}

void repondreCallback(String callbackId, String texte) {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + token + "/answerCallbackQuery?callback_query_id=" + callbackId + "&text=" + urlEncode(texte);
  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Connection: close");
  client.println();
  delay(100);
  client.stop();
}

String urlEncode(String str) {
  String encoded = "";
  char c;
  char buf[4];
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

// ====================================================================
// CAPTURE + ENVOI PHOTO (avec bouton "J'ai vu" — cas alerte réelle)
// ====================================================================
void capturerEtEnvoyerAlerte() {
  envoyerPhoto(true);
}

void capturerEtEnvoyerPhotoSimple() {
  envoyerPhoto(false);
}

void envoyerPhoto(bool avecBouton) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Échec capture photo");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  if (client.connect("api.telegram.org", 443)) {
    String caption = "🚨 ALERTE — " + String(currentMode == MODE_LASER ? "Laser coupé" : "Porte ouverte") +
                      "\nHeure : " + getHorodatage() +
                      "\nSignal Wi-Fi : " + String(WiFi.RSSI()) + " dBm";
    if (!avecBouton) caption = "📸 Photo à la demande - " + getHorodatage();

    String replyMarkup = "";
    if (avecBouton) {
      replyMarkup = "{\"inline_keyboard\":[[{\"text\":\"✅ J'ai vu\",\"callback_data\":\"ack\"}]]}";
    }

    String head = "--AlarmeBoundary\r\nContent-Disposition: form-data; name=\"chat_id\";\r\n\r\n" + chat_id + "\r\n";
    head += "--AlarmeBoundary\r\nContent-Disposition: form-data; name=\"caption\";\r\n\r\n" + caption + "\r\n";
    if (avecBouton) {
      head += "--AlarmeBoundary\r\nContent-Disposition: form-data; name=\"reply_markup\";\r\n\r\n" + replyMarkup + "\r\n";
    }
    head += "--AlarmeBoundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--AlarmeBoundary--\r\n";

    uint32_t totalLen = head.length() + fb->len + tail.length();

    client.println("POST /bot" + token + "/sendPhoto HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Type: multipart/form-data; boundary=AlarmeBoundary");
    client.print("Content-Length: ");
    client.println(totalLen);
    client.println();
    client.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t remaining = fb->len;
    while (remaining > 0) {
      size_t chunkSize = (remaining > 1024) ? 1024 : remaining;
      client.write(fbBuf, chunkSize);
      fbBuf += chunkSize;
      remaining -= chunkSize;
    }
    client.print(tail);

    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000) {
      while (client.available()) {
        client.read();
        timeout = millis();
      }
    }
  }

  client.stop();
  esp_camera_fb_return(fb);
}