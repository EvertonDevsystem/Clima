#include <Servo.h>
#include <DHT.h>
#include <NewPing.h>

// --- Definição dos pinos ---
const int PIN_VERMELHO = 2;
const int PIN_AMARELO   = 3;
const int PIN_VERDE     = 4;
const int PIN_SERVO     = 5;
const int PIN_ECHO      = 18;  // Ajustado para pinos compatíveis com ESP32
const int PIN_TRIG      = 19;  // Ajustado para pinos compatíveis com ESP32

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

void setupSensores() {
  servo1.attach(PIN_SERVO);
  servo1.write(posicaoServo);

  pinMode(PIN_VERMELHO, OUTPUT);
  pinMode(PIN_AMARELO, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);

  dht.begin();
}

void atualizarLogicaSensores() {
  // Controle do Servo por millis()
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnteriorServo >= intervaloServo) {
    tempoAnteriorServo = tempoAtual;
    posicaoServo = (posicaoServo == 0) ? 180 : 0;
    servo1.write(posicaoServo);
  }
}

// Função chamada pelo servidor do ESP32 para retornar os dados em XML
String obterXMLSensores() {
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
  
  return content;
}
