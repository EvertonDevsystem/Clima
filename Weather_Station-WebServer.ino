#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "index.h"  // Certifique-se de que o HTML acima está salvo neste arquivo de cabeçalho

WebServer server(80);
 
// Configuração do Sensor DHT22 (Temperatura e Umidade)
#include <DHT.h>
#define DHTPIN 4       // Pino GPIO conectado ao DHT22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Configuração do Sensor Ultrassônico (Simulação de Vento)
#include <NewPing.h>
#define trigger 19
#define echo 18
#define max_distancia 300
NewPing sonar(trigger, echo, max_distancia);

// Insira as credenciais da sua rede Wi-Fi
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
 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {      
    delay(500);
    Serial.print(".");
  }
    
  Serial.println("\nConectado com sucesso!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());  

  server.on("/", handleRoot);      
  server.on("/readADC", handleADC);
 
  server.begin();                  
  Serial.println("Servidor HTTP iniciado!");
  
  dht.begin();
}
 
void loop() {   
  server.handleClient();
  delay(1);
}

// Resposta XML contendo a Temperatura, Umidade e Vento separados
void XML_response() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (isnan(temperature)) temperature = 0.0;
  if (isnan(humidity)) humidity = 0.0;
  
  // Leitura do sensor ultrassônico convertida em escala para o medidor de vento
  unsigned int distancia = sonar.convert_cm(sonar.ping_median(5));
  float velocidadeVento = (distancia > 0 && distancia < 90) ? distancia : 10; 

  String content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  content += "<inputs>";
  content += "<analog>" + String(temperature, 1) + "</analog>";
  content += "<analog>" + String(humidity, 1) + "</analog>";
  content += "<analog>" + String(velocidadeVento, 1) + "</analog>";
  content += "</inputs>"; 
   
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/xml", content);
}
