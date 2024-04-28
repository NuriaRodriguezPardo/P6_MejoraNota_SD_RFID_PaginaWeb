#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

#define SS_PIN 5 // Pin del ESP8266/ESP32 conectado al SS del MFRC522
#define RST_PIN 21 // Pin del ESP8266/ESP32 conectado al RST del MFRC522
// #define MFRC522_SS_PIN 10  // Pin de Slave Select para el lector RFID
#define MFRC522_RST_PIN 21 // Pin de reset para el lector RFID
// #define SD_SS_PIN 5        // Pin de Slave Select para la tarjeta SD

#define MOSI_PIN 13 // Pin MOSI
#define MISO_PIN 12 // Pin MISO
#define SCK_PIN 11  // Pin SCK

#define WIFI_SSID "RedmiNuria"
#define WIFI_PASSWORD "Patata123"
// #define SERVER_IP "192.168.1.100" // Cambiar a la dirección IP de tu servidor web
#define SERVER_PORT 80

String obtenerHora();
void enviarDatosWeb(String uid, String hora);
void handleRoot();
//void escribirEnArchivo(String uid, String hora);
//void inicializarArchivo();

WebServer server(80); // Object of WebServer(HTTP port, 80 is defult)
MFRC522 mfrc522(SS_PIN, RST_PIN); // Crear instancia del lector RFID
File archivo;

void setup() {
  Serial.begin(115200);
  SPI.begin();       // Inicializar bus SPI
  mfrc522.PCD_Init(); // Inicializar lector RFID

  // Inicializar conexión WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  delay(100);

  // Inicializar archivo de log
  //inicializarArchivo();
}

void loop() {
  // Verificar si hay una tarjeta RFID presente
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Leer UID de la tarjeta RFID
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      uid.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    uid.toUpperCase();

    // Obtener la hora actual
    String hora = obtenerHora();

    // Escribir datos en el archivo de log
    //escribirEnArchivo(uid, hora);

    // Enviar datos a la página web
    enviarDatosWeb(uid, hora);
  }
}

void enviarDatosWeb(String uid, String hora) {
  // Establecer la conexión con el servidor
  WiFiClient client;
  if (!client.connect(WiFi.localIP(), SERVER_PORT)) {
    Serial.println("Error al conectar con el servidor");
    return;
  }

  // Construir el mensaje HTTP
  String mensaje = "GET /enviar_datos?uid=" + uid + "&hora=" + hora + " HTTP/1.1\r\n";
  mensaje += "Host: " + String(WiFi.localIP()) + "\r\n";
  mensaje += "Connection: close\r\n\r\n";

  // Enviar el mensaje HTTP
  client.print(mensaje);
  delay(100); // Esperar un momento para que se envíe completamente

  // Leer y mostrar la respuesta del servidor
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
}

void inicializarArchivo() {
  archivo = SD.open("/fichero.log", FILE_WRITE);
  if (!archivo) {
    Serial.println("Error al abrir el archivo");
    return;
  }
  archivo.println("Inicio de registro:");
  archivo.close();
}

void escribirEnArchivo(String uid, String hora) {
  archivo = SD.open("/fichero.log", FILE_APPEND);
  if (!archivo) {
    Serial.println("Error al abrir el archivo");
    return;
  }
  archivo.println("UID: " + uid + " - Hora: " + hora);
  archivo.close();
}

String obtenerHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Error al obtener la hora");
    return "";
  }

  char hora[20];
  strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(hora);
}

String paginaHTML = "<!DOCTYPE html>\
                      <html>\
                      <head>\
                      <meta charset='UTF-8'>\
                      <meta name='viewport' content='width=device-width, initial-scale=1.0'>\
                      <title>Información RFID</title>\
                      <script>\
                      function actualizarDatos(uid, hora) {\
                        document.getElementById('uid').innerHTML = uid;\
                        document.getElementById('hora').innerHTML = hora;\
                      }\
                      </script>\
                      </head>\
                      <body>\
                      <h1>Información RFID</h1>\
                      <p>UID: <span id='uid'></span></p>\
                      <p>Hora: <span id='hora'></span></p>\
                      </body>\
                      </html>";
                      
void handleRoot() {
  server.send(200, "text/html", paginaHTML);
}