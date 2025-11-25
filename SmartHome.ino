#include "esp_camera.h"
#include "board_config.h"

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
#define DISTANCE_THRESHOLD 100  // Distancia en cm para activar el foco (ajustable)

// ===========================
// Configuración de la Fotorresistencia
// ===========================
#define LDR_PIN 33        // Pin de la fotorresistencia (LDR)

// Variables para evitar múltiples capturas
unsigned long lastCaptureTime = 0;
const unsigned long CAPTURE_INTERVAL = 3000; // 3 segundos entre capturas
unsigned long lastUltrasonicCheck = 0;
const unsigned long ULTRASONIC_CHECK_INTERVAL = 500; // Revisar cada 500ms

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n=== Smart Home Security Camera ===");

  // Configurar pin del sensor PIR
  pinMode(PIR_SENSOR_PIN, INPUT);
  
  // Configurar pines del sensor ultrasónico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Configurar pin del relé (inicialmente apagado)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // LOW = relé apagado (foco apagado)
  
  // Configurar pin de la fotorresistencia
  pinMode(LDR_PIN, INPUT);
  
  // Configurar LED flash (asegurarse que esté apagado)
  pinMode(LED_FLASH_PIN, OUTPUT);
  digitalWrite(LED_FLASH_PIN, LOW); // LOW = apagado, HIGH = encendido
  
  // Pequeña pausa para estabilizar el sensor PIR
  Serial.println("Esperando estabilización del sensor PIR (2 segundos)...");
  delay(2000);

  // Configurar cámara
  if (!initCamera()) {
    Serial.println("Error al inicializar la cámara");
    ESP.restart();
  }

  Serial.println("Sistema listo. Esperando detección de movimiento...");
  Serial.printf("Sensor ultrasónico configurado. Distancia de activación: %d cm\n", DISTANCE_THRESHOLD);
}

void loop() {
  // Leer sensor PIR
  int pirState = digitalRead(PIR_SENSOR_PIN);

  // Verificar si hay movimiento (LOW = movimiento detectado en tu sensor)
  // Tu sensor funciona de manera invertida: LED IR encendido = movimiento = LOW
  if (pirState == LOW) {
    unsigned long currentTime = millis();
    
    // Verificar que haya pasado suficiente tiempo desde la última captura
    if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL) {
      Serial.println("\n¡MOVIMIENTO DETECTADO! (LED IR encendido) Capturando foto...");
      
      // Capturar foto con flash
      capturePhoto();
      
      // Actualizar tiempo de última captura
      lastCaptureTime = currentTime;
      
      // Pausa para evitar múltiples capturas seguidas
      delay(500);
    }
  }

  // Verificar sensor ultrasónico periódicamente
  unsigned long currentTime = millis();
  if (currentTime - lastUltrasonicCheck >= ULTRASONIC_CHECK_INTERVAL) {
    float distance = getDistance();
    
    // Leer estado de la fotorresistencia
    int ldrState = digitalRead(LDR_PIN);
    
    // Si la fotorresistencia detecta luz (LOW), apagar el foco
    if (ldrState == LOW) {
      digitalWrite(RELAY_PIN, LOW);
      static bool wasForcedOff = false;
      if (!wasForcedOff) {
        Serial.println("☀️ Luz detectada (fotorresistencia) - Foco APAGADO");
        wasForcedOff = true;
      }
    } else {
      // Si no hay luz ambiente, usar el sensor ultrasónico para controlar el foco
      if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
        // Objeto detectado cerca - encender foco
        digitalWrite(RELAY_PIN, HIGH);
        Serial.printf("💡 OBJETO DETECTADO a %.1f cm - Foco ENCENDIDO\n", distance);
      } else {
        // No hay objeto cerca - apagar foco
        digitalWrite(RELAY_PIN, LOW);
        // Solo imprimir cuando cambie el estado para no saturar el Serial
        static bool wasOn = false;
        if (wasOn) {
          Serial.println("💡 Foco APAGADO");
          wasOn = false;
        }
        if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
          wasOn = true;
        }
      }
    }
    
    lastUltrasonicCheck = currentTime;
  }

  delay(100); // Pequeño delay para no saturar el loop
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
  // Animación de flash: parpadeo rápido antes de la foto
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_FLASH_PIN, HIGH);
    delay(80);
    digitalWrite(LED_FLASH_PIN, LOW);
    delay(80);
  }
  Serial.println("⚡ Animación de flash realizada");

  // Encender LED flash para iluminar la escena
  digitalWrite(LED_FLASH_PIN, HIGH);
  Serial.println("⚡ Flash encendido");
  delay(220); // Tiempo para que el LED ilumine bien la escena

  // Capturar imagen
  camera_fb_t *fb = esp_camera_fb_get();

  // Apagar LED flash inmediatamente
  digitalWrite(LED_FLASH_PIN, LOW);
  Serial.println("⚡ Flash apagado");

  if (!fb) {
    Serial.println("❌ Error al capturar imagen");
    return;
  }

  Serial.printf("✓ Imagen capturada: %d bytes (%dx%d)\n", fb->len, fb->width, fb->height);

  // Liberar memoria del frame buffer
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
