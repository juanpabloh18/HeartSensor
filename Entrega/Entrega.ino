#include <Wire.h>                // Biblioteca para la comunicación I2C utilizada por el sensor MAX30105.
#include <ESP8266WiFi.h>         // Biblioteca para la conexión WiFi en el ESP8266.
#include <ESP8266WebServer.h>    // Biblioteca para crear un servidor web en el ESP8266.
#include "MAX30105.h"            // Biblioteca para manejar el sensor de frecuencia cardíaca MAX30105.
#include "heartRate.h"           // Biblioteca para el cálculo de latidos por minuto (BPM).

// Credenciales de red WiFi
const char* ssid     = "VIOLETA";        // Nombre de la red WiFi.
const char* password = "Botina18";       // Contraseña de la red WiFi.

// Configuración del servidor
const char* serverHost = "192.168.1.18"; // IP del servidor (ejemplo XAMPP) para recibir los datos.
const int serverPort = 80;               // Puerto del servidor, usualmente 80 para HTTP.

// Objetos de sensor y servidor
MAX30105 particleSensor;                 // Inicialización del objeto del sensor de frecuencia cardíaca.
ESP8266WebServer server(80);             // Configuración del servidor web en el ESP8266 en el puerto 80.

// Variables para la medición de frecuencia cardíaca
const byte RATE_SIZE = 8;                // Tamaño del buffer para almacenar valores de BPM.
byte rates[RATE_SIZE];                   // Arreglo para almacenar los valores de BPM.
long intervals[RATE_SIZE];               // Arreglo para almacenar los intervalos entre latidos.
byte rateSpot = 0;                       // Posición actual en el buffer de tasas de BPM.
long lastBeat = 0;                       // Registro del tiempo en que ocurrió el último latido.

float beatsPerMinute;                    // Variable para almacenar la frecuencia cardíaca en BPM.
int beatAvg;                             // Variable para almacenar el promedio de BPM.
unsigned long startMillis;               // Registro del tiempo de inicio de la medición.
const unsigned long measurementTime = 30000; // Tiempo total de medición en milisegundos (30 segundos).
bool measuring = false;                  // Bandera para indicar si está en proceso de medición.
bool measurementComplete = false;        // Bandera para indicar si la medición se completó.

float frecuencia;                        // Variable para almacenar la frecuencia resultante.
String estado;                           // Variable para almacenar el estado emocional calculado.

void setup() {
  Serial.begin(115200);                  // Inicia la comunicación serial con una velocidad de 115200 baudios.
  WiFi.begin(ssid, password);            // Conecta el ESP8266 a la red WiFi.

  // Espera hasta que el dispositivo se conecte a la red WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);                          // Espera 500 ms y muestra un punto en el monitor serial.
    Serial.print(".");
  }
  Serial.println("\nConexión WiFi establecida"); // Muestra un mensaje una vez que se conecta a WiFi.

  // Inicialización del sensor MAX30105
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Sensor MAX30102 no encontrado"); // Mensaje si el sensor no se detecta.
    while (1);                                      // Detiene el programa si no se encuentra el sensor.
  }
  particleSensor.setup();                           // Configuración inicial del sensor.
  particleSensor.setPulseAmplitudeRed(0x0A);        // Configura la amplitud del LED rojo para medir el pulso.
  particleSensor.setPulseAmplitudeGreen(0);         // Desactiva el LED verde (no necesario para esta medición).

  // Configura el servidor web para responder en la ruta /iniciar
  server.on("/iniciar", []() {
    server.send(200, "text/plain", "Tomando datos..."); // Respuesta al cliente al acceder a /iniciar.
    tomarDatos();                                      // Llama a la función para tomar datos de frecuencia cardíaca.
    enviarDatos();                                     // Llama a la función para enviar datos al servidor.
  });

  server.begin();                                      // Inicia el servidor web en el ESP8266.
  Serial.println("Servidor ESP8266 iniciado");         // Mensaje de confirmación de servidor iniciado.
}

void loop() {
  server.handleClient();   // Atiende solicitudes del cliente en el servidor.
}

// Función para tomar los datos de frecuencia cardíaca y HRV
void tomarDatos() {
  Serial.println("Tomando datos...");
  measuring = true;                      // Activa la bandera de medición.
  measurementComplete = false;           // Indica que la medición aún no está completa.
  rateSpot = 0;                          // Reinicia la posición del buffer de tasas de BPM.
  beatAvg = 0;                           // Reinicia el promedio de BPM.
  startMillis = millis();                // Registra el tiempo de inicio de la medición.

  // Bucle que dura el tiempo de medición (30 segundos)
  while (millis() - startMillis < measurementTime) {
    long irValue = particleSensor.getIR(); // Lee el valor de luz infrarroja para detectar la presencia de un dedo.

    // Detecta si hay un dedo colocado sobre el sensor
    if (!measuring && irValue > 50000) {
      measuring = true;                   // Activa la medición.
      Serial.println("Dedo detectado, comenzando medición...");
    }

    // Si hay un latido detectado, calcula el tiempo entre latidos
    if (measuring && checkForBeat(irValue)) {
      long delta = millis() - lastBeat;    // Calcula el intervalo de tiempo entre latidos.
      lastBeat = millis();                 // Actualiza el tiempo del último latido.

      beatsPerMinute = 60 / (delta / 1000.0); // Calcula el BPM en función del intervalo.

      // Solo considera valores válidos de BPM (entre 20 y 255)
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot] = (byte)beatsPerMinute;   // Almacena el BPM en el arreglo.
        intervals[rateSpot] = delta;              // Almacena el intervalo en el arreglo.
        rateSpot = (rateSpot + 1) % RATE_SIZE;    // Avanza en el buffer de forma circular.

        // Calcula el promedio de BPM
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
      }
    }
  }
  
  measuring = false;                  // Desactiva la medición.
  measurementComplete = true;         // Indica que la medición ha finalizado.
  Serial.println("\nMedición completada.");
  Serial.print("Promedio BPM: ");     // Muestra el BPM promedio en el monitor serial.
  Serial.println(beatAvg);

  // Calcula la variabilidad de frecuencia cardíaca (HRV)
  long hrv = 0;
  for (byte i = 1; i < RATE_SIZE; i++) {
    hrv += abs(intervals[i] - intervals[i - 1]);   // Suma la diferencia absoluta entre intervalos.
  }
  hrv /= (RATE_SIZE - 1);                          // Promedia la variabilidad entre latidos.

  // Determina el estado emocional basado en BPM y HRV
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
  Serial.println(estado);    // Muestra el estado emocional en el monitor serial.
}

// Función para enviar datos de frecuencia cardíaca y estado al servidor
void enviarDatos() {
  WiFiClient client;    // Crea un objeto cliente para la conexión.

  // Intenta conectar con el servidor
  if (!client.connect(serverHost, serverPort)) {
    Serial.println("Error al conectar con el servidor"); // Mensaje de error si no se conecta.
    return;
  }

  // Crea la URL con parámetros de frecuencia cardíaca, estado y origen.
  String url = "/frecuencia.php?frecuencia=";
  url += String(beatAvg);      // Agrega el promedio de BPM.
  url += "&estado=";
  url += estado;               // Agrega el estado emocional.
  url += "&source=ESP";        // Especifica el origen de los datos (ESP).

  // Realiza una solicitud GET al servidor
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + serverHost + "\r\n" +
               "Connection: close\r\n\r\n");

  // Muestra el resultado en el monitor serial
  Serial.println("Datos enviados: Promedio BPM = " + String(beatAvg) + ", Estado = " + estado);
  Serial.println("Solicitud enviada");
}
