#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "MAX30105.h"
#include "heartRate.h"

const char* ssid     = "VIOLETA";
const char* password = "Botina18";
const char* serverHost = "192.168.1.18"; // IP del servidor XAMPP
const int serverPort = 80;

MAX30105 particleSensor;
ESP8266WebServer server(80);

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
long intervals[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;
unsigned long startMillis;
const unsigned long measurementTime = 30000;
bool measuring = false;
bool measurementComplete = false;

float frecuencia;
String estado;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexión WiFi establecida");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Sensor MAX30102 no encontrado");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Ruta para iniciar la toma de datos
  server.on("/iniciar", []() {
    server.send(200, "text/plain", "Tomando datos...");
    tomarDatos();
    enviarDatos();
  });

  server.begin();
  Serial.println("Servidor ESP8266 iniciado");
}

void loop() {
  server.handleClient();
}

void tomarDatos() {
  Serial.println("Tomando datos...");
  measuring = true;
  measurementComplete = false;
  rateSpot = 0;
  beatAvg = 0;
  startMillis = millis();

  while (millis() - startMillis < measurementTime) {
    long irValue = particleSensor.getIR();

    if (!measuring && irValue > 50000) {
      measuring = true;
      Serial.println("Dedo detectado, comenzando medición...");
    }

    if (measuring && checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot] = (byte)beatsPerMinute;
        intervals[rateSpot] = delta;
        rateSpot = (rateSpot + 1) % RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  }

  measuring = false;
  measurementComplete = true;
  Serial.println("\nMedición completada.");
  Serial.print("Promedio BPM: ");
  Serial.println(beatAvg);

  // Calculo de HRV
  long hrv = 0;
  for (byte i = 1; i < RATE_SIZE; i++) {
    hrv += abs(intervals[i] - intervals[i - 1]);
  }
  hrv /= (RATE_SIZE - 1);

  // Clasificación del estado emocional
  if (beatAvg >= 60 && beatAvg <= 80) {
    estado = "Normal";
  } else if (beatAvg > 80 && beatAvg <= 100) {
    estado = "Estrés";
  } else if (beatAvg > 100) {
    if (hrv < 50) {
      estado = "Ira";
    } else {
      estado = "Estrés";
    }
  } else if (beatAvg < 60) {
    estado = "Depresión";
  }

  Serial.print("Estado: ");
  Serial.println(estado);
}

void enviarDatos() {
  WiFiClient client;
  if (!client.connect(serverHost, serverPort)) {
    Serial.println("Error al conectar con el servidor");
    return;
  }

  // Envía el Promedio BPM (beatAvg) en lugar de valores individuales de frecuencia
  String url = "/frecuencia.php?frecuencia=";
  url += String(beatAvg);
  url += "&estado=";
  url += estado;
  url += "&source=ESP";

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + serverHost + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("Datos enviados: Promedio BPM = " + String(beatAvg) + " bpm, Estado = " + estado);
}
