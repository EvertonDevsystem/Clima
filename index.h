#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <NewPing.h>
#include <Servo.h>

// Pinos
const int PIN_VERMELHO = 2;
const int PIN_AMARELO  = 3;
const int PIN_VERDE    = 4;
const int PIN_SERVO    = 5;
const int TRIGGER_PIN  = 19;
const int ECHO_PIN     = 18;

// Sensores e Atuadores
Servo servo1;
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, 300);

// Credenciais Wi-Fi e ThingSpeak
const char* ssid = "Rede";
const char* password = "Senha";
const char* serverAddress = "api.thingspeak.com";
String writeAPIKey = "SUA_WRITE_API_KEY_AQUI"; // Cole sua Write API Key do ThingSpeak aqui

WebServer server(80);
unsigned long ultimoEnvio = 0;
const long intervaloEnvio = 20000;
unsigned long tempoAnteriorServo = 0;
int posicaoServo = 0;

void handleRoot() {
  // O conteúdo HTML é carregado diretamente pelo index.html do navegador ou servido pelo PROGMEM se preferir.
}

// Resposta XML enviada para a página web com as 4 variáveis
void XML_response() {
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  float vento = 15.0; 
  unsigned int distancia = sonar.convert_cm(sonar.ping_median(5));

  if (isnan(temperatura)) temperatura = 0.0;
  if (isnan(umidade)) umidade = 0.0;

  String content = "<?xml version=\"1.0\" ?>";
  content += "<inputs>";
  content += "<val>" + String(temperatura, 1) + "</val>";
  content += "<val>" + String(umidade, 1) + "</val>";
  content += "<val>" + String(vento, 1) + "</val>";
  content += "<val>" + String(distancia) + "</val>";
  content += "</inputs>";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/xml", content);
}

void setupESP32() {
  pinMode(PIN_VERMELHO, OUTPUT);
  pinMode(PIN_AMARELO, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);
  
  servo1.attach(PIN_SERVO);
  servo1.write(posicaoServo);
  
  dht.begin();
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  server.on("/readSensors", XML_response);
  server.begin();
}

void loopESP32() {
  server.handleClient();

  // Movimentação do Servo sem travar o ESP32 (Millis)
  if (millis() - tempoAnteriorServo >= 2000) {
    tempoAnteriorServo = millis();
    posicaoServo = (posicaoServo == 0) ? 180 : 0;
    servo1.write(posicaoServo);
  }

  // Envio periódico para o ThingSpeak a cada 20 segundos
  if (millis() - ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = millis();
    
    WiFiClient client;
    if (client.connect(serverAddress, 80)) {
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      float v = 15.0;
      unsigned int d = sonar.convert_cm(sonar.ping_median(5));

      String postStr = writeAPIKey;
      postStr += "&field1=" + String(t);
      postStr += "&field2=" + String(h);
      postStr += "&field3=" + String(v);
      postStr += "&field4=" + String(d);
      postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
      client.stop();
    }
  }
}
