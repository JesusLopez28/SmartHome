#include "esp_camera.h"
#include "board_config.h"
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include "base64.h"

// ===========================
// Configuración WiFi
// ===========================
const char* ssid = "Totalplay-B7A2";        
const char* password = "bza8g=M1eiAc0a1";

// ===========================
// Configuración Firebase
// ===========================
const char* firebaseHost = "https://smarthome-4d3c6-default-rtdb.firebaseio.com";
const char* firebaseAuth = "AIzaSyDDrOCcoZbBVXCFfImSrJyfBFjCGMNOj98";
const char* deviceId = "camara1";

// Configuración de zona horaria
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -21600;  // GMT-6 (México Centro)
const int daylightOffset_sec = 0;   // Sin horario de verano

// ===========================
// Configuración del sensor PIR
// ===========================
#define PIR_SENSOR_PIN 13  // Cambia este pin según tu conexión
#define LED_FLASH_PIN 12   // LED flash de la cámara (GPIO 12)

// ===========================
// Configuración del sensor Ultrasónico
// ===========================
#define TRIG_PIN 14        // Pin Trigger del sensor ultrasónico
#define ECHO_PIN 15        // Pin Echo del sensor ultrasónico
#define RELAY_PIN 2        // Pin del relé para controlar el foco
#define DISTANCE_THRESHOLD 40  // Distancia en cm para activar el foco (ajustable)

// ===========================
// Configuración de la Fotorresistencia
// ===========================
#define LDR_PIN 33        // Pin de la fotorresistencia (LDR)

// Prototipo de función para evitar error de declaración
void sendSensorEvent(String sensorType, String event, float value = 0);

// Variables para evitar múltiples capturas
unsigned long lastCaptureTime = 0;
const unsigned long CAPTURE_INTERVAL = 3000; // 3 segundos entre capturas
unsigned long lastUltrasonicCheck = 0;
const unsigned long ULTRASONIC_CHECK_INTERVAL = 500; // Revisar cada 500ms

// Variables de estado para el control del relé
bool relayState = false; // Estado actual del relé (false = apagado, true = encendido)
bool lightDetected = false; // Estado de la fotorresistencia

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   SMART HOME SECURITY CAMERA v1.0    ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  // Conectar a WiFi
  Serial.println("┌─ Conectando a WiFi");
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print("│ .");
    wifiAttempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("└─ ✓ WiFi conectado exitosamente");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    
    // Sincronizar RTC
    Serial.println("┌─ Sincronizando reloj...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.print("└─ ✓ Hora actual: ");
    printLocalTime();
    Serial.println("\n");
  } else {
    Serial.println("└─ ⚠ ERROR: No se pudo conectar a WiFi\n");
  }

  // Configurar pines
  Serial.println("┌─ Configurando sensores y actuadores:");
  pinMode(PIR_SENSOR_PIN, INPUT);
  Serial.println("│  ✓ Sensor PIR configurado (GPIO 13)");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.printf("│  ✓ Sensor Ultrasónico configurado (TRIG: GPIO %d, ECHO: GPIO %d)\n", TRIG_PIN, ECHO_PIN);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Serial.printf("│  ✓ Relé configurado y apagado (GPIO %d)\n", RELAY_PIN);
  
  pinMode(LDR_PIN, INPUT);
  Serial.printf("│  ✓ Fotorresistencia configurada (GPIO %d)\n", LDR_PIN);
  
  pinMode(LED_FLASH_PIN, OUTPUT);
  digitalWrite(LED_FLASH_PIN, LOW);
  Serial.printf("│  ✓ LED Flash configurado (GPIO %d)\n", LED_FLASH_PIN);
  Serial.println("└─ Sensores listos\n");
  
  // Pequeña pausa para estabilizar el sensor PIR
  Serial.println("⏳ Estabilizando sensor PIR (2 segundos)...");
  delay(2000);

  // Configurar cámara
  Serial.println("┌─ Inicializando cámara...");
  if (!initCamera()) {
    Serial.println("└─ ❌ ERROR CRÍTICO: Fallo al inicializar cámara");
    Serial.println("   Reiniciando sistema...\n");
    ESP.restart();
  }
  Serial.println("└─ ✓ Cámara lista\n");

  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║      SISTEMA INICIADO Y LISTO         ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.printf("\n📍 Distancia de activación: %d cm\n", DISTANCE_THRESHOLD);
  Serial.println("👁️  Esperando detección de movimiento...\n");
  Serial.println("─────────────────────────────────────────\n");
}

void loop() {
  // Leer sensor PIR
  int pirState = digitalRead(PIR_SENSOR_PIN);

  // Verificar si hay movimiento
  if (pirState == LOW) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL) {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.print("║  🚨 MOVIMIENTO DETECTADO - ");
      printLocalTime();
      Serial.println("\n╚════════════════════════════════════════╝");
      
      sendSensorEvent("PIR", "movimiento_detectado");
      capturePhotoAndSend();
      
      lastCaptureTime = currentTime;
      Serial.println("─────────────────────────────────────────\n");
      delay(500);
    }
  }

  // Verificar sensor ultrasónico periódicamente
  unsigned long currentTime = millis();
  if (currentTime - lastUltrasonicCheck >= ULTRASONIC_CHECK_INTERVAL) {
    float distance = getDistance();
    
    // Leer estado de la fotorresistencia
    int ldrValue = digitalRead(LDR_PIN);
    bool currentLightDetected = (ldrValue == LOW);
    
    // Verificar cambio en estado de luz
    if (currentLightDetected != lightDetected) {
      lightDetected = currentLightDetected;
      if (lightDetected) {
        // Hay luz ambiente - forzar apagado del relé
        if (relayState) {
          digitalWrite(RELAY_PIN, LOW);
          relayState = false;
          Serial.println("\n┌─────────────────────────────────────────┐");
          Serial.print("│ ☀️  LUZ DETECTADA - ");
          printLocalTime();
          Serial.println("\n│ 💡 Foco APAGADO (control automático)");
          Serial.println("└─────────────────────────────────────────┘\n");
          sendSensorEvent("LDR", "luz_detectada_foco_apagado");
        }
      } else {
        Serial.println("\n🌙 Oscuridad detectada - Modo ultrasónico activado\n");
      }
    }
    
    // Si NO hay luz ambiente, usar el sensor ultrasónico
    if (!lightDetected && distance > 0) {
      bool shouldBeOn = (distance <= DISTANCE_THRESHOLD);
      
      // Solo cambiar estado si es diferente al actual
      if (shouldBeOn != relayState) {
        relayState = shouldBeOn;
        digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
        
        Serial.println("\n┌─────────────────────────────────────────┐");
        Serial.print("│ 📏 Distancia: ");
        Serial.printf("%.1f cm - ", distance);
        printLocalTime();
        
        if (relayState) {
          Serial.println("\n│ 💡 Foco ENCENDIDO (objeto detectado)");
          sendSensorEvent("Ultrasonico", "objeto_detectado_foco_encendido", distance);
        } else {
          Serial.println("\n│ 💡 Foco APAGADO (sin objeto cercano)");
          sendSensorEvent("Ultrasonico", "objeto_fuera_rango_foco_apagado", distance);
        }
        Serial.println("└─────────────────────────────────────────┘\n");
      }
    }
    
    lastUltrasonicCheck = currentTime;
  }

  delay(100);
}

bool initCamera() {
  Serial.println("Inicializando cámara...");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  
  // Configuración para mejor calidad de imagen
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // 800x600
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Inicializar cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error en esp_camera_init: 0x%x\n", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  // Ajustes de imagen
  s->set_brightness(s, 0);     // -2 a 2
  s->set_contrast(s, 0);       // -2 a 2
  s->set_saturation(s, 0);     // -2 a 2
  s->set_special_effect(s, 0); // 0 a 6 (0 = Sin efecto)
  s->set_whitebal(s, 1);       // Balance de blancos
  s->set_awb_gain(s, 1);       // Ganancia automática balance blancos
  s->set_wb_mode(s, 0);        // Modo balance blancos
  s->set_exposure_ctrl(s, 1);  // Control de exposición automático
  s->set_aec2(s, 0);           // AEC sensor
  s->set_ae_level(s, 0);       // -2 a 2
  s->set_aec_value(s, 300);    // 0 a 1200
  s->set_gain_ctrl(s, 1);      // Control de ganancia automático
  s->set_agc_gain(s, 0);       // 0 a 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 a 6
  s->set_bpc(s, 0);            // Black pixel correction
  s->set_wpc(s, 1);            // White pixel correction
  s->set_raw_gma(s, 1);        // Gamma correction
  s->set_lenc(s, 1);           // Lens correction
  s->set_hmirror(s, 0);        // Espejo horizontal
  s->set_vflip(s, 0);          // Volteo vertical
  s->set_dcw(s, 1);            // DCW (downsize enable)
  s->set_colorbar(s, 0);       // Barra de color de prueba

  Serial.println("Cámara inicializada correctamente");
  return true;
}

void capturePhoto() {
  Serial.println("┌─ Preparando captura:");
  // Animación de flash: parpadeo rápido antes de la foto
  Serial.print("│  ⚡ Flash: ");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_FLASH_PIN, HIGH);
    delay(80);
    digitalWrite(LED_FLASH_PIN, LOW);
    delay(80);
    Serial.print("💫 ");
  }
  Serial.println();

  // Encender LED flash para iluminar la escena
  digitalWrite(LED_FLASH_PIN, HIGH);
  Serial.println("│  🔦 Iluminando escena...");
  delay(220);

  // Capturar imagen
  camera_fb_t *fb = esp_camera_fb_get();

  // Apagar LED flash inmediatamente
  digitalWrite(LED_FLASH_PIN, LOW);
  Serial.println("│  ⚡ Flash apagado");

  if (!fb) {
    Serial.println("└─ ❌ ERROR: Fallo al capturar imagen\n");
    return;
  }

  Serial.printf("└─ ✓ Captura exitosa: %d bytes (%dx%d)\n\n", fb->len, fb->width, fb->height);
  esp_camera_fb_return(fb);
}

void capturePhotoAndSend() {
  Serial.println("┌─ Iniciando captura con flash:");
  // Animación de flash
  Serial.print("│  ⚡ Animación: ");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_FLASH_PIN, HIGH);
    delay(80);
    digitalWrite(LED_FLASH_PIN, LOW);
    delay(80);
    Serial.print("💫 ");
  }
  Serial.println();

  digitalWrite(LED_FLASH_PIN, HIGH);
  Serial.println("│  🔦 Iluminando escena...");
  delay(220);

  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(LED_FLASH_PIN, LOW);
  Serial.println("│  ⚡ Flash apagado");

  if (!fb) {
    Serial.println("└─ ❌ ERROR: Fallo en captura\n");
    sendSensorEvent("Camara", "error_captura_imagen");
    return;
  }

  Serial.printf("│  ✓ Imagen: %d bytes (%dx%d)\n", fb->len, fb->width, fb->height);
  Serial.println("└─ Procesando...\n");
  
  sendPhotoToFirebase(fb);
  esp_camera_fb_return(fb);
}

float getDistance() {
  // Limpiar el pin trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Enviar pulso de 10 microsegundos
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Leer el tiempo de respuesta del pin echo
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout de 30ms
  
  // Calcular distancia en centímetros
  // Velocidad del sonido: 343 m/s = 0.0343 cm/µs
  // Distancia = (tiempo * velocidad) / 2
  if (duration == 0) {
    return -1; // Error o fuera de rango
  }
  
  float distance = duration * 0.0343 / 2;
  
  return distance;
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.print("--:--:--");
    return;
  }
  Serial.printf("%02d:%02d:%02d", 
                timeinfo.tm_hour, 
                timeinfo.tm_min, 
                timeinfo.tm_sec);
}

// ===========================
// Funciones Firebase
// ===========================
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return String(millis());
  }
  char timestamp[30];
  sprintf(timestamp, "%04d-%02d-%02d_%02d:%02d:%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday,
          timeinfo.tm_hour,
          timeinfo.tm_min,
          timeinfo.tm_sec);
  return String(timestamp);
}

void sendToFirebase(String path, String jsonData) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("   ❌ WiFi desconectado. No se puede enviar a Firebase.");
    return;
  }

  HTTPClient http;
  String url = String(firebaseHost) + path + ".json?auth=" + String(firebaseAuth);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  Serial.println("┌─ Enviando a Firebase:");
  int httpResponseCode = http.POST(jsonData);
  
  if (httpResponseCode > 0) {
    Serial.printf("└─ ✓ Respuesta: %d - Datos enviados correctamente\n\n", httpResponseCode);
  } else {
    Serial.printf("└─ ❌ Error %d: %s\n\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
  }
  
  http.end();
}

void sendPhotoToFirebase(camera_fb_t *fb) {
  if (!fb) return;
  
  Serial.println("┌─ Codificando imagen:");
  Serial.print("│  📊 Procesando Base64");
  String imageBase64 = base64::encode(fb->buf, fb->len);
  Serial.println(" ✓");
  
  String timestamp = getTimestamp();
  
  String jsonData = "{";
  jsonData += "\"device\":\"" + String(deviceId) + "\",";
  jsonData += "\"timestamp\":\"" + timestamp + "\",";
  jsonData += "\"type\":\"motion_detected\",";
  jsonData += "\"width\":" + String(fb->width) + ",";
  jsonData += "\"height\":" + String(fb->height) + ",";
  jsonData += "\"size\":" + String(fb->len) + ",";
  jsonData += "\"image\":\"" + imageBase64 + "\"";
  jsonData += "}";
  
  Serial.printf("│  📦 Tamaño JSON: %d bytes\n", jsonData.length());
  Serial.println("└─ Listo para enviar\n");
  sendToFirebase("/eventos/fotos", jsonData);
}

void sendSensorEvent(String sensorType, String event, float value) {
  String timestamp = getTimestamp();
  
  String jsonData = "{";
  jsonData += "\"device\":\"" + String(deviceId) + "\",";
  jsonData += "\"timestamp\":\"" + timestamp + "\",";
  jsonData += "\"sensor\":\"" + sensorType + "\",";
  jsonData += "\"event\":\"" + event + "\"";
  
  if (value != 0) {
    jsonData += ",\"value\":" + String(value, 2);
  }
  
  jsonData += "}";
  
  sendToFirebase("/eventos/sensores", jsonData);
}
