#include <WiFi.h>
#include <Preferences.h>
#include <HX711.h>
#include <WebSocketsClient.h>  // Requiere instalar librería "WebSockets" de Markus Sattler
#include <ArduinoJson.h>       // Requiere instalar librería "ArduinoJson" de Benoit Blanchon

// ==========================================
// 1. CONSTANTES Y PINES
// ==========================================
#define DT1 32   
#define SCK1 33  
#define PIN_COMPRESION 34       
#define BOTON_TRANSMISION 17     

// Credenciales Wi-Fi
const char* WIFI_SSID = "Ancoltec SAS";
const char* WIFI_PASSWORD = "sai90086241"; 

// Credenciales WebSocket
// Cambia la IP por la dirección IPv4 de tu servidor Node.js
const char* WS_HOST = "192.168.11.8"; 
const uint16_t WS_PORT = 3000;
const char* WS_PATH = "/";

// ==========================================
// 2. VARIABLES GLOBALES Y OBJETOS
// ==========================================
HX711 celda1;
Preferences preferences;
WebSocketsClient webSocket;

// Estado y Control
float escalaCelda1 = 22.04;            
bool transmitir = false;             
unsigned long lastTransmitTime = 0;  
const unsigned long transmitInterval = 200; // 200ms -> 5Hz (Igual que en Node.js)

// ==========================================
// 3. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(BOTON_TRANSMISION, INPUT_PULLUP);

  setupWiFi();
  setupWebSocket();
  
  celda1.begin(DT1, SCK1);
  forzarNuevoFactor();
  
  mostrarMenu();
}

// ==========================================
// 4. LOOP PRINCIPAL
// ==========================================
void loop() {
  mantenerConexionWiFi(); 
  webSocket.loop();       // Mantiene viva la conexión WebSocket (Heartbeat)
  manejarBotones();       
  manejarSerial();        
  transmitirDatos();      
}

// ==========================================
// 5. FUNCIONES DE RED (WIFI & WEBSOCKET)
// ==========================================
void setupWiFi() {
  Serial.println("\nConectando a Ancoltec SAS...");
  WiFi.mode(WIFI_STA); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n¡WiFi Conectado! IP Asignada: ");
  Serial.println(WiFi.localIP());
}

void mantenerConexionWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
  }
}

void setupWebSocket() {
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  
  // Asignar la función que escuchará los mensajes entrantes
  webSocket.onEvent(manejarEventosWebSocket);
  
  // Intentar reconectar cada 5 segundos si se cae el servidor
  webSocket.setReconnectInterval(5000);
}

// Escucha activa de mensajes desde el servidor Node.js
void manejarEventosWebSocket(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Desconectado del servidor.");
      break;
      
    case WStype_CONNECTED:
      Serial.printf("[WS] Conectado exitosamente al servidor: %s\n", WS_HOST);
      break;
      
    case WStype_TEXT:
      // Cuando recibimos un texto (JSON), lo analizamos con ArduinoJson
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        // Extraemos el comando
        const char* comando = doc["command"];
        
        // Si el comando existe y es "tare"
        if (comando && strcmp(comando, "tare") == 0) {
          Serial.println("---> [WS] COMANDO TARA RECIBIDO! Tarando balanza...");
          celda1.tare();
        }
      } else {
        Serial.print("[WS] Error parseando JSON entrante: ");
        Serial.println(error.c_str());
      }
      break;
  }
}

// ==========================================
// 6. MANEJO DE ENTRADAS FÍSICAS Y SERIAL
// ==========================================
void manejarBotones() {
  static unsigned long lastDebounce = 0;
  if (digitalRead(BOTON_TRANSMISION) == LOW && (millis() - lastDebounce > 300)) {
    celda1.tare(); 
    transmitir = true;
    lastDebounce = millis();
  }
}

void manejarSerial() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim(); 
    comando.toUpperCase(); 

    if (comando == "S") transmitir = true;
    else if (comando == "P") transmitir = false;
    else if (comando == "T") {
      celda1.tare();
      Serial.println("Balanza tarada a 0.000 Kg");
    }
    else if (comando == "C1") {
      calibrarCelda(celda1, "Celda 1");
      guardarCalibraciones();
    } 
    else if (comando == "V") mostrarCalibraciones();
    else if (comando == "M") mostrarMenu();
  }
}

// ==========================================
// 7. MEDICIÓN Y TRANSMISIÓN
// ==========================================
void transmitirDatos() {
  if (transmitir && (millis() - lastTransmitTime >= transmitInterval)) {
    lastTransmitTime = millis(); 

    if (celda1.is_ready()) {
      // Como estamos transmitiendo a 5Hz, promediar 50 valores podría ser muy lento.
      // Si la celda se laguea, reduce el "10" a un número menor (ej. 3 o 5).
      float peso_bruto = celda1.get_units(10); 
      float peso_kg = peso_bruto / 1000.0;
      
      // Filtro de ruido
      if (peso_kg <= 0.01 && peso_kg >= -0.01) peso_kg = 0.0;

      // 1. Mostrar en monitor Serial
      Serial.print("*");
      Serial.print(peso_kg, 3);
      Serial.println(" Kg");

      // 2. Enviar por WebSocket si está conectado
      if (webSocket.isConnected()) {
        StaticJsonDocument<100> doc;
        
        // El formato queda: {"weight": "120.000"} para igualar tu Node.js
        doc["weight"] = String(peso_kg, 3); 
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        webSocket.sendTXT(jsonPayload);
      }
    }
  }
}

// ==========================================
// 8. MEMORIA Y CALIBRACIÓN
// ==========================================
void forzarNuevoFactor() {
  preferences.begin("calibraciones", false); 
  escalaCelda1 = 22.04; 
  preferences.putFloat("escala1", escalaCelda1);
  preferences.end();
  
  celda1.set_scale(escalaCelda1);
  Serial.print("Memoria actualizada. Factor de escala: ");
  Serial.println(escalaCelda1, 5);
}

void calibrarCelda(HX711 &celda, const char *nombre) {
  transmitir = false; 
  Serial.println("--- Modo Calibración ---");
  Serial.println("1. Quite el peso y envíe 'Enter'");
  while(Serial.available()) Serial.read(); 
  while (!Serial.available()); 
  celda.tare(); 

  Serial.println("2. Ponga peso conocido y envíe 'Enter'");
  while(Serial.available()) Serial.read(); 
  while (!Serial.available()); 

  Serial.println("3. Ingrese el peso en GRAMOS:");
  while (!Serial.available());
  float pesoConocido = Serial.parseFloat();

  if (pesoConocido != 0) {
    escalaCelda1 = celda.get_units(10) / pesoConocido; 
    celda.set_scale(escalaCelda1);         
    Serial.print("Nuevo factor: ");
    Serial.println(escalaCelda1, 5);
  }
}

void guardarCalibraciones() {
  preferences.begin("calibraciones", false); 
  preferences.putFloat("escala1", escalaCelda1);
  preferences.end();
}

void mostrarCalibraciones() {
  Serial.print("Factor de escala actual: ");
  Serial.println(escalaCelda1, 5);
}

void mostrarMenu() {
  Serial.println("\n--- CONTROL DE BALANZA ---");
  Serial.println("S: Iniciar | P: Parar | T: Tarar | C1: Calibrar | V: Ver Escala | M: Menu");
}