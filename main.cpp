#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQTT.h>
#include "secrets.h"
#include <DNSServer.h>

// Definições do sensor NTC
#define THERMISTOR_PIN A0
#define R1 10000.0
#define K1 1.009249522e-03
#define K2 2.378405444e-04
#define K3 2.019202697e-07

// Definições do sensor Dallas
#define ONE_WIRE_BUS D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#ifdef USE_SUMMER_TIME_DST
uint8_t DST = 1;
#else
uint8_t DST = 0;
#endif

// Variáveis do NTC
float tempNTC = 0.0;
float offsetNTC = 0.0;

// Variáveis de configuração do dispositivo
struct DeviceSettings {
    String deviceId ;
    String equipment;
    String client = "bio";
    float offsetNTC = 0.0;
};

DeviceSettings deviceSettings;

// Variáveis de rede e MQTT
const int MQTT_PORT = 8883;
const char MQTT_SUB_TOPIC[] = "$aws/things/myespwork/shadow/name/sensor/get";
const char MQTT_PUB_TOPIC[] = "$aws/things/myespwork/shadow/name/sensor";
WiFiClientSecure net;
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
MQTTClient client(256);
unsigned long lastMillis = 0;
time_t now;
time_t nowish = 1510592825;




// Servidor Web
ESP8266WebServer server(80);

#ifdef USE_PUB_SUB
void messageReceivedPubSub(char *topic, byte *payload, unsigned int length) {
  Serial.print("Recebido [");
  Serial.print(topic);
  Serial.println("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void pubSubErr(int8_t MQTTErr) {
  if (MQTTErr == MQTT_CONNECTION_TIMEOUT)
    Serial.print("Connection timeout");
  else if (MQTTErr == MQTT_CONNECTION_LOST)
    Serial.print("Connection lost");
  else if (MQTTErr == MQTT_CONNECT_FAILED)
    Serial.print("Connect failed");
  else if (MQTTErr == MQTT_DISCONNECTED)
    Serial.print("Disconnected");
  else if (MQTTErr == MQTT_CONNECTED)
    Serial.print("Connected");
  else if (MQTTErr == MQTT_CONNECT_BAD_PROTOCOL)
    Serial.print("Connect bad protocol");
  else if (MQTTErr == MQTT_CONNECT_BAD_CLIENT_ID)
    Serial.print("Connect bad Client-ID");
  else if (MQTTErr == MQTT_CONNECT_UNAVAILABLE)
    Serial.print("Connect unavailable");
  else if (MQTTErr == MQTT_CONNECT_BAD_CREDENTIALS)
    Serial.print("Connect bad credentials");
  else if (MQTTErr == MQTT_CONNECT_UNAUTHORIZED)
    Serial.print("Connect unauthorized");
}

#else
void messageReceived(String &topic, String &payload) {
  Serial.println("Recebido [" + topic + "]:");
  Serial.println(payload);
}

void lwMQTTErrConnection(lwmqtt_return_code_t reason) {
  if (reason == lwmqtt_return_code_t::LWMQTT_CONNECTION_ACCEPTED)
    Serial.print("Connection Accepted");
  else if (reason == lwmqtt_return_code_t::LWMQTT_UNACCEPTABLE_PROTOCOL)
    Serial.print("Unacceptable Protocol");
  else if (reason == lwmqtt_return_code_t::LWMQTT_IDENTIFIER_REJECTED)
    Serial.print("Identifier Rejected");
  else if (reason == lwmqtt_return_code_t::LWMQTT_SERVER_UNAVAILABLE)
    Serial.print("Server Unavailable");
  else if (reason == lwmqtt_return_code_t::LWMQTT_BAD_USERNAME_OR_PASSWORD)
    Serial.print("Bad UserName/Password");
  else if (reason == lwmqtt_return_code_t::LWMQTT_NOT_AUTHORIZED)
    Serial.print("Not Authorized");
  else if (reason == lwmqtt_return_code_t::LWMQTT_UNKNOWN_RETURN_CODE)
    Serial.print("Unknown Return Code");
}
#endif

void connectToWiFi(String init_str) {
  if (init_str != "") {
    Serial.print(init_str);
  }
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  if (init_str != "") {
    Serial.println(" WiFi Conectado...");
  }
}

unsigned long previousMillis = 0;
const long interval = 10000;

// Funções de conexão e configuração
void NTPConnect() {
  Serial.print("Configurando Horário: ");
  configTime(TIME_ZONE * 3600, DST * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("Concluído!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Horário: ");
  Serial.print(asctime(&timeinfo));
}

void lwMQTTErr(lwmqtt_err_t reason) {
  if (reason == lwmqtt_err_t::LWMQTT_SUCCESS)
    Serial.print("Success");
  else if (reason == lwmqtt_err_t::LWMQTT_BUFFER_TOO_SHORT)
    Serial.print("Buffer too short");
  else if (reason == lwmqtt_err_t::LWMQTT_VARNUM_OVERFLOW)
    Serial.print("Varnum overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_CONNECT)
    Serial.print("Network failed connect");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_TIMEOUT)
    Serial.print("Network timeout");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_READ)
    Serial.print("Network failed read");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_WRITE)
    Serial.print("Network failed write");
  else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_OVERFLOW)
    Serial.print("Remaining length overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_MISMATCH)
    Serial.print("Remaining length mismatch");
  else if (reason == lwmqtt_err_t::LWMQTT_MISSING_OR_WRONG_PACKET)
    Serial.print("Missing or wrong packet");
  else if (reason == lwmqtt_err_t::LWMQTT_CONNECTION_DENIED)
    Serial.print("Connection denied");
  else if (reason == lwmqtt_err_t::LWMQTT_FAILED_SUBSCRIPTION)
    Serial.print("Failed subscription");
  else if (reason == lwmqtt_err_t::LWMQTT_SUBACK_ARRAY_OVERFLOW)
    Serial.print("Suback array overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_PONG_TIMEOUT)
    Serial.print("Pong timeout");
}

void connectToMqtt(bool nonBlocking = false) {
  Serial.print("Conectando ao AWS IoT Core MQTT: ");
  while (!client.connected()) {
    if (client.connect(THINGNAME)) {
      Serial.println("Conectado!");
      if (!client.subscribe(MQTT_SUB_TOPIC))
        lwMQTTErr(client.lastError());
    } else {
      Serial.print("Falha. Motivo -> ");
      lwMQTTErrConnection(client.returnCode());

      if (!nonBlocking) {
        Serial.println(" < Nova tentativa em 5 segundos");
        delay(5000);
      } else {
        Serial.println(" <");
      }
    }
    if (nonBlocking)
      break;
  }
}

void checkWiFiThenMQTT() {
  connectToWiFi("Checking WiFi");
  connectToMqtt();
}

// Função de leitura do sensor NTC
float readNTCTemperature() {
  int adcValue = analogRead(THERMISTOR_PIN);
  float resistance = (1023.0 / adcValue - 1.0) * R1;
  float logR = log(resistance);
  float tempK = 1.0 / (K1 + K2 * logR + K3 * logR * logR * logR);
  float tempC = tempK - 273.15;
  return tempC + offsetNTC;
}

// Função de envio de dados
void sendData() {
  JsonDocument doc;
  
  doc["deviceId"] = deviceSettings.deviceId;
  doc["equipment"] = deviceSettings.equipment;
  doc["client"] = deviceSettings.client;
  doc["temperature"] = tempNTC; 
  doc["offsetNTC"] = offsetNTC;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  Serial.println(jsonBuffer); // Para verificar o JSON gerado no console
  Serial.print("Tamanho do JSON: ");
  Serial.println(strlen(jsonBuffer));
  if (!client.publish(MQTT_PUB_TOPIC, jsonBuffer, false, 1))
    lwMQTTErr(client.lastError());
}


void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Calibracao e Configuracao do Dispositivo</h1>";
  html += "<form action=\"/submit\" method=\"POST\">";
  html += "<label for=\"offsetNTC\">Offset NTC:</label>";
  html += "<input type=\"number\" step=\"0.01\" id=\"offsetNTC\" name=\"offsetNTC\" value=\"" + String(offsetNTC) + "\"><br><br>";
  html += "<label for=\"client\">Client:</label>";
  html += "<input type=\"text\" id=\"client\" name=\"client\" value=\"" + deviceSettings.client + "\"><br><br>";
  html += "<label for=\"deviceId\">Device ID:</label>";
  html += "<input type=\"text\" id=\"deviceId\" name=\"deviceId\" value=\"" + deviceSettings.deviceId + "\"><br><br>";
  html += "<label for=\"equipment\">Equipment:</label>";
  html += "<input type=\"text\" id=\"equipment\" name=\"equipment\" value=\"" + deviceSettings.equipment + "\"><br><br>";
  html += "<input type=\"submit\" value=\"Salvar\">";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleFormSubmit() {
  if (server.hasArg("offsetNTC")) {
    offsetNTC = server.arg("offsetNTC").toFloat();
  }
  if (server.hasArg("client")) {
    deviceSettings.client = server.arg("client");
  }
  if (server.hasArg("deviceId")) {
    deviceSettings.deviceId = server.arg("deviceId");
  }
  if (server.hasArg("equipment")) {
    deviceSettings.equipment = server.arg("equipment");
  }
  server.send(200, "text/plain", "Informacoes salvas!");
}

void setup() {
  Serial.begin(9600);

  // Configuração do Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  connectToWiFi("Conectando com o WiFi Rede: " + String(ssid) + "....");

  // Configuração do horário via NTP
  NTPConnect();

  // Inicialização dos sensores
  sensors.begin();

  // Configuração do cliente MQTT seguro
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
  client.begin(MQTT_HOST, MQTT_PORT, net);
  client.onMessage(messageReceived);

  // Conexão ao MQTT
  connectToMqtt();

  // Inicialização do servidor web
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleFormSubmit);
  server.begin();
  Serial.println("Servidor Web iniciado.");
}

void loop() {
  client.loop();
  delay(10);  // Pequena pausa para permitir a execução de outras tarefas
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Leitura da temperatura do NTC
    tempNTC = readNTCTemperature();
    Serial.print("Temperatura NTC: ");
    Serial.println(tempNTC);

    // Leitura da temperatura dos sensores Dallas
    sensors.requestTemperatures();
    float tempDallas = sensors.getTempCByIndex(0);
    Serial.print("Temperatura Dallas: ");
    Serial.println(tempDallas);

    // Envio dos dados para o AWS IoT Core
    sendData();
  }
  server.handleClient();
  //checkWiFiThenMQTT();
}
