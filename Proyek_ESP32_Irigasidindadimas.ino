/*
  Proyek 4A - Embedded System Agroteknologi
  Board  : WeMos D1 R32 / ESP32
  Sensor : DHT21 + Soil Moisture Analog
  Output : LCD 20x4 I2C + ThingSpeak + Web API
  Kontrol: Relay 4 Channel, digunakan 2 tombol web

  Wiring:
  DHT21 DATA          -> GPIO4
  Soil Moisture AO    -> GPIO34

  LCD I2C 20x4:
  SDA -> GPIO21
  SCL -> GPIO22
  VCC -> 5V
  GND -> GND

  Relay:
  Relay 1 IN -> GPIO17
  Relay 2 IN -> GPIO26
  Relay 3 IN -> GPIO25
  Relay 4 IN -> GPIO33

  Web API:
  /data
  /relay?ch=1&state=1
  /relay?ch=1&state=0
  /relay?ch=2&state=1
  /relay?ch=2&state=0
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ThingSpeak.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <elapsedMillis.h>

// ===============================
// GANTI BAGIAN INI SESUAI PUNYA LU
// ===============================
const char* ssid = "UGMURO-INET";
const char* password = "Gepuk15000";

const char* writeAPIKey = "YIPE4HL7OH12HWME";
const unsigned long channelID = 3399912;
// ===============================

// Pin sensor
#define DHTPIN 4
#define DHTTYPE DHT21
#define SOIL_PIN 36

// Pin relay 4 channel
#define RELAY1_PIN 17
#define RELAY2_PIN 26
#define RELAY3_PIN 25
#define RELAY4_PIN 16

// Kalau relay lu nyala kebalik, ubah jadi true
// Banyak relay module itu ACTIVE LOW
const bool RELAY_ACTIVE_LOW = false;

// Kalau mau relay otomatis berdasarkan sensor, ubah jadi true
// Kalau mau relay dikontrol dari tombol website, biarkan false
const bool AUTO_CONTROL = true;

// Batas otomatis
const int SOIL_DRY_LIMIT = 30;       // Pompa ON jika kelembapan tanah < 30%
const int TEMP_LIMIT = 24;           // Relay 1 ON jika suhu > 24
const int HUMIDITY_LIMIT = 75;       // Relay 1 ON jika humidity > 75

// Interval waktu
unsigned long thingSpeakInterval = 15000;
unsigned long sensorInterval = 500;
unsigned long displayInterval = 1000;

elapsedMillis thingSpeakMillis;
elapsedMillis sensorMillis;
elapsedMillis displayMillis;

// Object
WiFiClient client;
WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variabel sensor
int soilValue = 0;
int soilPercentage = 0;
float temperature = 0;
float humidity = 0;

// Status relay
bool relay1State = false;
bool relay2State = false;
bool relay3State = false;
bool relay4State = false;

// Status upload
String uploadStatus = "Upload: -";

// ===============================
// FUNGSI RELAY
// ===============================

void setRelayPin(int pin, bool state) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, state ? LOW : HIGH);
  } else {
    digitalWrite(pin, state ? HIGH : LOW);
  }
}

void setRelay(int channel, bool state) {
  if (channel == 1) {
    relay1State = state;
    setRelayPin(RELAY1_PIN, state);
  } 
  else if (channel == 2) {
    relay2State = state;
    setRelayPin(RELAY2_PIN, state);
  } 
  else if (channel == 3) {
    relay3State = state;
    setRelayPin(RELAY3_PIN, state);
  } 
  else if (channel == 4) {
    relay4State = state;
    setRelayPin(RELAY4_PIN, state);
  }
}

void allRelayOff() {
  setRelay(1, false);
  setRelay(2, false);
  setRelay(3, false);
  setRelay(4, false);
}

// ===============================
// FUNGSI SENSOR
// ===============================

void readSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  delay(10);

  soilValue = analogRead(SOIL_PIN);

  // Sensor analog soil biasanya:
  // nilai besar = kering
  // nilai kecil = basah
  soilPercentage = map(soilValue, 4095, 0, 0, 100);
  soilPercentage = constrain(soilPercentage, 0, 100);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C | Humidity: ");
  Serial.print(humidity);
  Serial.print(" % | Soil ADC: ");
  Serial.print(soilValue);
  Serial.print(" | Soil: ");
  Serial.print(soilPercentage);
  Serial.println(" %");
}

// ===============================
// KONTROL OTOMATIS OPSIONAL
// ===============================

void autoControlRelay() {
  if (AUTO_CONTROL == false) {
    return;
  }

  // Relay 1 otomatis berdasarkan suhu dan kelembapan udara
  if (!isnan(temperature) && !isnan(humidity)) {
    if (temperature > TEMP_LIMIT && humidity > HUMIDITY_LIMIT) {
      setRelay(1, true);
    } else {
      setRelay(1, false);
    }
  } else {
    setRelay(1, false);
  }

  // Relay 2 sebagai pompa otomatis berdasarkan kelembapan tanah
  if (soilPercentage < SOIL_DRY_LIMIT) {
    setRelay(2, true);
  } else {
    setRelay(2, false);
  }
}

// ===============================
// LCD
// ===============================

void lcdWelcome() {
  lcd.clear();

  lcd.setCursor(3, 0);
  lcd.print("Selamat Datang!");

  lcd.setCursor(0, 1);
  lcd.print("WS Agroteknologi IoT");

  lcd.setCursor(3, 3);
  lcd.print("-- UG MURO --");

  delay(3000);
  lcd.clear();
}

void lcdShowMainLabel() {
  lcd.clear();

  lcd.setCursor(5, 0);
  lcd.print("Monitoring");

  lcd.setCursor(0, 1);
  lcd.print("Suhu   : ");

  lcd.setCursor(0, 2);
  lcd.print("K.Udara: ");

  lcd.setCursor(0, 3);
  lcd.print("K.Tanah: ");
}

void updateLCD() {
  lcd.setCursor(5, 0);
  lcd.print("Monitoring");

  lcd.setCursor(9, 1);
  lcd.print("       ");
  lcd.setCursor(9, 1);

  if (isnan(temperature)) {
    lcd.print("Err");
  } else {
    lcd.print(temperature, 1);
  }

  lcd.setCursor(16, 1);
  lcd.print(char(223));
  lcd.print("C");

  lcd.setCursor(9, 2);
  lcd.print("       ");
  lcd.setCursor(9, 2);

  if (isnan(humidity)) {
    lcd.print("Err");
  } else {
    lcd.print(humidity, 1);
  }

  lcd.setCursor(17, 2);
  lcd.print("%");

  lcd.setCursor(9, 3);
  lcd.print("       ");
  lcd.setCursor(9, 3);
  lcd.print(soilPercentage);

  lcd.setCursor(17, 3);
  lcd.print("%");
}

// ===============================
// WIFI
// ===============================

void connectWiFi() {
  Serial.println();
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");
  lcd.setCursor(0, 1);
  lcd.print(ssid);

  WiFi.begin(ssid, password);

  int retry = 0;

  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("IP ESP32: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");

    lcd.setCursor(0, 1);
    lcd.print("IP:");

    lcd.setCursor(0, 2);
    lcd.print(WiFi.localIP());

    delay(3000);
  } else {
    Serial.println();
    Serial.println("WiFi gagal terhubung");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Gagal");

    lcd.setCursor(0, 1);
    lcd.print("Cek SSID/PASS");

    delay(3000);
  }
}

// ===============================
// THINGSPEAK
// ===============================

void uploadThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    uploadStatus = "WiFi Error";
    Serial.println("WiFi tidak terhubung. Upload dibatalkan.");
    return;
  }

  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, soilPercentage);
  ThingSpeak.setField(4, soilValue);
  ThingSpeak.setField(5, relay1State ? 1 : 0);
  ThingSpeak.setField(6, relay2State ? 1 : 0);

  int x = ThingSpeak.writeFields(channelID, writeAPIKey);

  if (x == 200) {
    uploadStatus = "OK";
    Serial.println("ThingSpeak update successful.");
  } else {
    uploadStatus = "Error " + String(x);
    Serial.println("ThingSpeak update failed. HTTP error code: " + String(x));
  }
}

// ===============================
// CORS UNTUK WEBSITE DARI VSCODE
// ===============================

void sendCorsHeader() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ===============================
// WEB API: /data
// ===============================

void handleData() {
  sendCorsHeader();

  String json = "{";

  json += "\"soil\":" + String(soilPercentage) + ",";
  json += "\"soilRaw\":" + String(soilValue) + ",";

  if (isnan(temperature)) {
    json += "\"temperature\":0,";
  } else {
    json += "\"temperature\":" + String(temperature, 1) + ",";
  }

  if (isnan(humidity)) {
    json += "\"humidity\":0,";
  } else {
    json += "\"humidity\":" + String(humidity, 1) + ",";
  }

  json += "\"relay1\":" + String(relay1State ? 1 : 0) + ",";
  json += "\"relay2\":" + String(relay2State ? 1 : 0) + ",";
  json += "\"relay3\":" + String(relay3State ? 1 : 0) + ",";
  json += "\"relay4\":" + String(relay4State ? 1 : 0);

  json += "}";

  server.send(200, "application/json", json);
}

// ===============================
// WEB API: /relay?ch=1&state=1
// ===============================

void handleRelay() {
  sendCorsHeader();

  if (!server.hasArg("ch") || !server.hasArg("state")) {
    server.send(400, "text/plain", "Parameter salah. Gunakan /relay?ch=1&state=1");
    return;
  }

  int channel = server.arg("ch").toInt();
  int state = server.arg("state").toInt();

  if (channel < 1 || channel > 4) {
    server.send(400, "text/plain", "Channel relay harus 1 sampai 4");
    return;
  }

  if (state != 0 && state != 1) {
    server.send(400, "text/plain", "State harus 0 atau 1");
    return;
  }

  setRelay(channel, state == 1);

  String response = "Relay ";
  response += String(channel);
  response += state == 1 ? " ON" : " OFF";

  Serial.println(response);

  server.send(200, "text/plain", response);
}

void handleRoot() {
  sendCorsHeader();

  String text = "ESP32 Web API aktif\n";
  text += "Buka /data untuk melihat JSON sensor\n";
  text += "Contoh: /relay?ch=1&state=1";

  server.send(200, "text/plain", text);
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/relay", HTTP_GET, handleRelay);

  server.begin();

  Serial.println("Web API ESP32 berjalan.");
  Serial.print("Cek data di: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/data");
}

// ===============================
// SETUP
// ===============================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // LCD
  lcd.init();
  lcd.backlight();
  lcdWelcome();

  // Pin sensor
  pinMode(SOIL_PIN, INPUT);

  // Pin relay
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  allRelayOff();

  // Sensor
  dht.begin();

  // ADC ESP32
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  // WiFi
  connectWiFi();

  // ThingSpeak
  ThingSpeak.begin(client);

  // Web API
  setupWebServer();

  // Tampilan utama LCD
  lcdShowMainLabel();

  // Baca sensor awal
  readSensors();
  updateLCD();
}

// ===============================
// LOOP
// ===============================

void loop() {
  // Wajib supaya ESP32 bisa menerima request dari website VSCode
  server.handleClient();

  // Reconnect WiFi kalau putus
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Baca sensor
  if (sensorMillis >= sensorInterval) {
    readSensors();
    autoControlRelay();
    sensorMillis = 0;
  }

  // Update LCD
  if (displayMillis >= displayInterval) {
    updateLCD();
    displayMillis = 0;
  }

  // Upload ThingSpeak
  if (thingSpeakMillis >= thingSpeakInterval) {
    uploadThingSpeak();
    thingSpeakMillis = 0;
  }
}