#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Servo.h>
#include <DHT.h>
#include <NewPing.h>
#include "index.h"  // Arquivo HTML da página web

WebServer server(80);

// --- Definição dos Pinos ---
const int PIN_VERMELHO = 2;
const int PIN_AMARELO   = 3;
const int PIN_VERDE     = 4;
const int PIN_SERVO     = 5;
const int PIN_ECHO      = 18;  // Pino Echo do ultrassônico
const int PIN_TRIG      = 19;  // Pino Trigger do ultrassônico

// --- Configuração do Sensor DHT22 (Temperatura e Umidade) ---
#define DHTPIN 4       
#define DHTTYPE DHT22  
DHT dht(DHTPIN, DHTTYPE);

// --- Configuração do Sensor Ultrassônico (Vento) ---
#define MAX_DISTANCE 300
NewPing sonar(PIN_TRIG, PIN_ECHO, MAX_DISTANCE);

// Instância do Servo
Servo servo1;
unsigned long tempoAnteriorServo = 0;
const long intervaloServo = 2000; 
int posicaoServo = 0;

// Credenciais da Rede Wi-Fi
const char* ssid = "Rede";
const char* password = "Senha";

void handleRoot() {
  String s = MAIN_page; 
  server.send(200, "text/html, charset=utf-8", s); 
}

void handleADC() {
  XML_response();
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nInicializando Estação Meteorológica...");

  // Inicialização dos componentes
  dht.begin();
  servo1.attach(PIN_SERVO);
  servo1.write(posicaoServo);

  pinMode(PIN_VERMELHO, OUTPUT);
  pinMode(PIN_AMARELO, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);

  // Conexão Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {      
    delay(500);
    Serial.print(".");
  }
    
  Serial.println("\nConectado com sucesso!");
  Serial.print("IP de Acesso: http://");
  Serial.println(WiFi.localIP());  

  server.on("/", handleRoot);      
  server.on("/readADC", handleADC);
 
  server.begin();                  
  Serial.println("Servidor HTTP iniciado!");
}

void loop() {   
  server.handleClient();
  
  // Controle não bloqueante do Servo (Millis)
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnteriorServo >= intervaloServo) {
    tempoAnteriorServo = tempoAtual;
    posicaoServo = (posicaoServo == 0) ? 180 : 0;
    servo1.write(posicaoServo);
  }
  
  delay(1);
}

// Resposta XML enviada para o site com os dados reais dos sensores
void XML_response() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (isnan(temperature)) temperature = 0.0;
  if (isnan(humidity)) humidity = 0.0;
  
  unsigned int distancia = sonar.convert_cm(sonar.ping_median(5));
  float velocidadeVento = (distancia > 0 && distancia < 90) ? (float)distancia : 10.0; 

  String content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  content += "<inputs>";
  content += "<analog>" + String(temperature, 1) + "</analog>";
  content += "<analog>" + String(humidity, 1) + "</analog>";
  content += "<analog>" + String(velocidadeVento, 1) + "</analog>";
  content += "</inputs>"; 
   
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/xml", content);
}
