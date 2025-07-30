#include <WiFi.h>
#include <PubSubClient.h>
#include <AccelStepper.h>
#include <HardwareSerial.h>
#include <ArduinoOTA.h>

// Pines de motores y control
#define DIR_PINX 27
#define STEP_PINX 14  

#define DIR_PINY 25
#define STEP_PINY 33

#define ENABLE_PIN 13

#define MS1_PIN 12
#define MS2_PIN 32 
#define MS3_PIN 26 

// Pines UART del sensor LP8
#define RX_uart1 16
#define TX_uart1 17
#define vbb_en1 5
#define rdy1 18

//BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
unsigned long lastMeasurement_bme = 0;  // Tiempo de la última medición
unsigned int measurementCount_bme = 0;  // Contador de mediciones
#define numero_mediciones_bme 5
#define tiempo_mediciones_bme 1000 
 float promedio[3];



// WiFi y MQTT
const char* ssid = "Oficina AG";
const char* password = "OficinaAG23";
const char* mqtt_server = "192.168.31.61";

WiFiClient espClient;
PubSubClient client(espClient);
char clientId[30];

// Motores
AccelStepper stepperX(AccelStepper::DRIVER, STEP_PINX, DIR_PINX);
AccelStepper stepperY(AccelStepper::DRIVER, STEP_PINY, DIR_PINY);

// UART LP8
HardwareSerial LP8_Serial1(1);

// Medición de CO2
unsigned long lastMeasurement = 0;
unsigned long measurementInterval = 20000;
bool continuousMeasurement = false;

// Heartbeat
unsigned long lastHeartbeat = 0;
unsigned long heartbeatInterval = 5000;

// Estado del sistema
int speedX = 0;
int speedY = 0;
bool dirX = true;
bool dirY = true;
bool motorsEnabled = false;
int microstepping = 1;

// Comandos LP8
static byte write_partida[] = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x10, 0x28, 0x7e};
static byte write_normal[] = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x20, 0x28, 0x6A};
static byte write_calibrar[] = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x53, 0x69, 0x8F};
static byte read_32_bytes[] = {0xfe, 0x44, 0x00, 0x80, 0x20, 0x79, 0x3C};
static byte read_44_bytes[] = {0xfe, 0x44, 0x00, 0x80, 0x2c, 0x79, 0x39};
static byte read_4_bytes[] = {0xfe, 0x44, 0x00, 0xA4, 0x4, 0x62, 0x27};
byte response[49];
int crc_result = 0;

void sendRequest(HardwareSerial &SerialPort, byte packet[]) {
  int n,m;
  if(packet[1] == 0x44){ //si es 0x44, es solicitud de read, si es 0x41 es solicitud de write
    n = packet[4] + 5; //en la ubicación packet[2] están la cantidad de datos solicitados a leer
    m = 7;
  }else
  {
  n = 4;
  m = 8;
  }
  
  // Enviar el paquete

  
    Serial.println("Esperando disponibilidad del puerto serial...");
    SerialPort.write(packet, m);
    
  

  // Esperar el byte de inicio FE
  unsigned long startTime = millis();
  bool startFound = false;
  
  
  while (millis() - startTime < 5000) {
    if (SerialPort.available()) {
      byte b = SerialPort.read();
      Serial.print("Byte de inicio leido:");
      Serial.println(b,HEX);
      if (b == 0xFE) {
        response[0] = b;
        Serial.println("Inicio de mensaje detectado: 0xFE");
        startFound = true;
        break;
      }
    }
    delay(10);
  }

  if (!startFound) {
    Serial.println("Timeout esperando byte de inicio FE.");
    return;
  }

  // Leer los siguientes n - 1 bytes con timeout
  int i = 1;
  startTime = millis();  // Reiniciar tiempo
      Serial.print("response[");
      Serial.print("0");
      Serial.print("] = ");
      Serial.println(response[0], HEX);
  while (i < n && millis() - startTime < 1000) {
    if (SerialPort.available()) {
      response[i] = SerialPort.read();
      Serial.print("response[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(response[i], HEX);
      i++;
    }
  }

  if (i < n) {
    Serial.println("Timeout leyendo respuesta completa.");
    return;
  }

  Serial.println("Mensaje completo recibido.");
  Serial.flush();  // Asegura que todo se ha impreso
}

void procesarLectura(int n) {
  
  // Calcular CRC
  crc_result = ModRTU_CRC(n-2);
  Serial.print("CRC calculado: ");
  Serial.println(crc_result, HEX);

  int crc_result_h = crc_result & 0xff;
  int crc_result_l = (crc_result >> 8) & 0xff;

  if ((response[n-2] != crc_result_h) || (response[n-1] != crc_result_l)) {
    Serial.println("! ! ! El CRC calculado no coincide con el CRC del sensor.");
    Serial.print("crc_High = ");
    Serial.println(response[n-2],HEX);
    Serial.print("crc_Low = ");
    Serial.println(response[n-1],HEX);
    
    for(int i = 0; i < n; i++){
      Serial.println(response[i],HEX);
    }

  } else {
    Serial.println("El CRC calculado coincide con el CRC del sensor.");
  }
}



int ModRTU_CRC(byte n1) {
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < n1; pos++) {
    crc ^= (uint16_t)response[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  Serial.print("CRC calculado: ");
  Serial.println(crc, HEX);
  return crc;
}
void mostrar_co2() {
  int co2 = 256 * response[29] + response[30];
  Serial.print("CO2: "); Serial.println(co2);
  char topic[64];
  sprintf(topic, "esp32/%s/lp8", clientId);
  char msg[16];
  sprintf(msg, "%d", co2);
  client.publish(topic, msg);
}

void setMicrostepping(int ms) {
  if (ms == 0) {
    digitalWrite(MS1_PIN, LOW);
    digitalWrite(MS2_PIN, LOW);
    digitalWrite(MS3_PIN, LOW);
  } else {
    digitalWrite(MS1_PIN, ms & 1);
    digitalWrite(MS2_PIN, (ms >> 1) & 1);
    digitalWrite(MS3_PIN, (ms >> 2) & 1);
  }
  microstepping = ms;
}

void enviar_status() {
  char topic[64];
  sprintf(topic, "esp32/%s/status", clientId);

  String status = "{";
  status += "\"motorX_speed\":" + String(speedX) + ",";
  status += "\"motorY_speed\":" + String(speedY) + ",";
  status += "\"motorX_dir\":" + String(dirX) + ",";
  status += "\"motorY_dir\":" + String(dirY) + ",";
  status += "\"microstepping\":" + String(microstepping) + ",";
  status += "\"motors_enabled\":" + String(motorsEnabled ? "true" : "false") + ",";
  status += "\"co2_monitoring\":" + String(continuousMeasurement ? "true" : "false");
  status += "}";

  client.publish(topic, status.c_str());
}

void reconnect() {
  while (!client.connected()) {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    sprintf(clientId, "%s", mac.c_str());
    if (client.connect(clientId)) {
      client.subscribe((String("esp32/") + clientId + "/motor/#").c_str());
      client.subscribe((String("esp32/") + clientId + "/sensor/measure").c_str());
      client.subscribe((String("esp32/") + clientId + "/sensor/stop").c_str());
    } else {
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String t = topic;
  String base = String("esp32/") + clientId + "/";
  String cmd = t.substring(base.length());
  payload[length] = 0;
  String val = String((char*)payload);

  if (cmd == "motor/speedX") speedX = val.toInt();
  else if (cmd == "motor/speedY") speedY = val.toInt();
  else if (cmd == "motor/directionX") dirX = val.toInt();
  else if (cmd == "motor/directionY") dirY = val.toInt();
  else if (cmd == "motor/microstepping") setMicrostepping(val.toInt());
  else if (cmd == "motor/start") {
    motorsEnabled = (val.toInt() == 1);
    digitalWrite(ENABLE_PIN, motorsEnabled ? LOW : HIGH);
    if (!motorsEnabled) {
      stepperX.setSpeed(0);
      stepperY.setSpeed(0);
    }
  }
  else if (cmd == "sensor/measure") {
    continuousMeasurement = true;
    lastMeasurement = 0;
  }
  else if (cmd == "sensor/stop") {
    continuousMeasurement = false;
  }

  // Siempre enviar status luego de cambios
  enviar_status();
}

void configureBME280() {
    if (!bme.begin(0x76)) {
        Serial.println("No se encontró el sensor BME280.");
        //while (1);
    }

    bme.setSampling(
        Adafruit_BME280::MODE_FORCED, 
        Adafruit_BME280::SAMPLING_X16, 
        Adafruit_BME280::SAMPLING_X16, 
        Adafruit_BME280::SAMPLING_X16, 
        Adafruit_BME280::FILTER_X16
    );

    Serial.println("BME280 configurado.");
}

void readBME280Data(float arr[]) {
    bme.takeForcedMeasurement();  // Tomar una nueva medición

    float temperatura = bme.readTemperature();
    float humedad = bme.readHumidity();
    float presion = bme.readPressure() / 100.0F;

    arr[0] = arr[0] + temperatura;
    arr[1] = arr[1] + humedad;
    arr[2] = arr[2] + presion;

    Serial.print("Temp: "); Serial.print(temperatura); Serial.println(" °C");
    Serial.print("Humedad: "); Serial.print(humedad); Serial.println(" %");
    Serial.print("Presión: "); Serial.print(presion); Serial.println(" hPa");
}

void publicarBME280(float promedio[]) {
  char topic[64];
  char msg[16];

  // Temperatura
  sprintf(topic, "esp32/%s/bme280/temperatura", clientId);
  dtostrf(promedio[0], 6, 2, msg);  // Convierte float a string
  client.publish(topic, msg);

  // Humedad
  sprintf(topic, "esp32/%s/bme280/humedad", clientId);
  dtostrf(promedio[1], 6, 2, msg);
  client.publish(topic, msg);

  // Presión
  sprintf(topic, "esp32/%s/bme280/presion", clientId);
  dtostrf(promedio[2], 7, 2, msg);  // Presión puede tener más dígitos
  client.publish(topic, msg);

  Serial.println("Datos BME280 enviados por MQTT en subtópicos.");
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Iniciando configuración de wi-fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
   Serial.printf("Iniciando configuración de mqqt_server");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

   Serial.printf("Iniciando configuración de pines motores");
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  setMicrostepping(microstepping);
  digitalWrite(ENABLE_PIN, HIGH);

   Serial.printf("Iniciando configuración de LP8");
  LP8_Serial1.begin(9600, SERIAL_8N1, RX_uart1, TX_uart1);
  pinMode(vbb_en1, OUTPUT);
  pinMode(rdy1, INPUT_PULLDOWN);

  stepperX.setMaxSpeed(2000);
  stepperY.setMaxSpeed(2000);

  //configuramos bme280
   Serial.printf("Iniciando configuración de bme280");
   configureBME280();

   measurementCount_bme = 0;
       promedio[0] = 0;  //temperatura
       promedio[1] = 0;  //Humedad
       promedio[2] = 0;  //Presión
  
   Serial.printf("Iniciando configuración de OTA");
  byte mac[6];
  WiFi.macAddress(mac);
  char value[80];
  sprintf(value,"%s-%02x%02x%02x",clientId,mac[2],mac[1],mac[0]);
  //Configuramos ArduinoOTA
  Serial.printf("ClientId :");
  Serial.printf(clientId);
  Serial.printf("Value:");
  Serial.printf(value);

  ArduinoOTA.setHostname(clientId);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_FS
      type = "filesystem";
    Serial.println("Inicio de actualización OTA: " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nFin de la actualización");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progreso: %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Fallo de autenticación");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Fallo al comenzar");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Fallo de conexión");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Fallo de recepción");
    else if (error == OTA_END_ERROR) Serial.println("Fallo al finalizar");
  });

  ArduinoOTA.setPassword("asdasdasd");
  ArduinoOTA.begin();
  Serial.println("OTA listo");
}

void loop() {
  Serial.printf("Estamos en el loop");
  ArduinoOTA.handle();  // Handles a code update request
  if (!client.connected()) reconnect();
  client.loop();

  // Heartbeat
  if (millis() - lastHeartbeat > heartbeatInterval) {
    char topic[64];
    sprintf(topic, "esp32/heartbeat/%s", clientId);
    client.publish(topic, "online");
    lastHeartbeat = millis();
  }

  // Movimiento de motores
  if (motorsEnabled) {
    stepperX.setSpeed(dirX ? speedX : -speedX);
    stepperX.runSpeed();
    stepperY.setSpeed(dirY ? speedY : -speedY);
    stepperY.runSpeed();
  }
   
  if(millis() - lastMeasurement_bme >= tiempo_mediciones_bme){
          readBME280Data(promedio);  // Leer datos del sensor
       Serial.print("Temperatura;");
       Serial.print(promedio[0]);
       Serial.println("C");
       Serial.print("Humedad:");
       Serial.print(promedio[1]);
       Serial.println("%");
       Serial.print("Presion:");
       Serial.print(promedio[2]);
       Serial.println("Hpa");
       publicarBME280(promedio);
       lastMeasurement_bme = millis();
  }
  
       
  // Medición de CO₂ periódica
if (continuousMeasurement && millis() - lastMeasurement >= measurementInterval) {
  Serial.println("=== Iniciando ciclo de medición de CO₂ ===");

  digitalWrite(vbb_en1, LOW);
  delay(1000);
  digitalWrite(vbb_en1, HIGH);
  delay(1000);

  // Esperar a que rdy1 se ponga en LOW
  unsigned long start = millis();
  bool rdyBajo = false;
  while (digitalRead(rdy1) != LOW && millis() - start < 500);
  if (digitalRead(rdy1) == LOW) {
    rdyBajo = true;
    Serial.println("rdy1 pasó a LOW correctamente");
  } else {
    Serial.println("⏱ Timeout esperando rdy1 en LOW");
  }

  sendRequest(LP8_Serial1, write_normal);

  // Esperar a que rdy1 vuelva a HIGH
  start = millis();
  bool rdyAlto = false;
  while (digitalRead(rdy1) != HIGH && millis() - start < 1000);
  if (digitalRead(rdy1) == HIGH) {
    rdyAlto = true;
    Serial.println("rdy1 volvió a HIGH correctamente");
  } else {
    Serial.println("⏱ Timeout esperando rdy1 en HIGH");
  }

  sendRequest(LP8_Serial1, read_44_bytes);
  procesarLectura(49);
  mostrar_co2();
  lastMeasurement = millis();

  Serial.println("=== Fin del ciclo de medición ===\n");
}

}
