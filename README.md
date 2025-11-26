# 🏠 Smart Home Security Camera

Sistema de seguridad inteligente basado en ESP32-CAM con detección de movimiento, control automático de iluminación y envío de eventos a Firebase.

![Version](https://img.shields.io/badge/version-1.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-orange.svg)
![License](https://img.shields.io/badge/license-Apache%202.0-green.svg)

## 📋 Tabla de Contenidos

- [Características](#-características)
- [Hardware Requerido](#-hardware-requerido)
- [Diagrama de Conexiones](#-diagrama-de-conexiones)
- [Configuración](#️-configuración)
- [Instalación](#-instalación)
- [Uso](#-uso)
- [API Firebase](#-api-firebase)
- [Funciones Principales](#-funciones-principales)
- [Troubleshooting](#-troubleshooting)
- [Licencia](#-licencia)

## ✨ Características

### 🎥 Detección y Captura
- **Detección de movimiento PIR**: Sensor infrarrojo pasivo para detectar presencia
- **Captura automática con flash**: Fotos de alta calidad (hasta 1600x1200) con iluminación LED
- **Anti-rebote**: Intervalo de 3 segundos entre capturas para evitar duplicados
- **Sincronización temporal**: NTP para timestamp preciso (GMT-6 México)

### 💡 Control Inteligente de Iluminación
- **Sensor ultrasónico HC-SR04**: Medición de distancia para activación automática
- **Fotorresistencia (LDR)**: Detección de luz ambiente
- **Control de relé**: Activación/desactivación automática de iluminación externa
- **Lógica inteligente**: Solo activa luces artificiales cuando NO hay luz natural

### 🌐 Conectividad
- **WiFi integrado**: Conexión a red doméstica
- **Firebase Realtime Database**: Almacenamiento en la nube
- **Envío de imágenes en Base64**: Codificación para almacenamiento web
- **Registro de eventos**: Logs detallados de sensores y acciones

### 📱 Modelos de Cámara Soportados
- ESP32 Wrover Kit (predeterminado)
- AI-Thinker ESP32-CAM
- M5Stack PSRAM/V2/Wide/ESP32CAM
- ESP32-S3 Eye
- XIAO ESP32S3
- Y más... (ver `board_config.h`)

## 🔧 Hardware Requerido

### Componentes Principales
| Componente | Modelo | Cantidad | Descripción |
|------------|--------|----------|-------------|
| Microcontrolador | ESP32-CAM (Wrover) | 1 | Con PSRAM para imágenes de alta resolución |
| Sensor PIR | HC-SR501 | 1 | Detección de movimiento infrarrojo |
| Sensor Ultrasónico | HC-SR04 | 1 | Medición de distancia (2-400 cm) |
| Fotorresistencia | LDR GL5528 | 1 | Detección de luz ambiente |
| Módulo Relé | 1 Canal 5V | 1 | Control de carga hasta 10A |
| LED Flash | LED Blanco de alta potencia | 1 | Iluminación para cámara |
| Resistencias | 10kΩ | 2 | Pull-down para LDR y PIR |

### Conexiones de Pines

#### ESP32-CAM (Wrover Kit)
```
┌─────────────────────────────────────┐
│         ESP32-CAM WROVER            │
├─────────────────────────────────────┤
│ GPIO 13 → Sensor PIR (OUT)          │
│ GPIO 12 → LED Flash                 │
│ GPIO 14 → Sensor Ultrasónico (TRIG) │
│ GPIO 15 → Sensor Ultrasónico (ECHO) │
│ GPIO 2  → Relé (IN)                 │
│ GPIO 33 → Fotorresistencia (LDR)    │
│                                     │
│ 5V   → Alimentación sensores        │
│ GND  → Tierra común                 │
└─────────────────────────────────────┘
```

#### Pines de Cámara (Configuración WROVER_KIT)
```
PWDN:  -1       Y9:  35       SIOD: 26
RESET: -1       Y8:  34       SIOC: 27
XCLK:  21       Y7:  39
VSYNC: 25       Y6:  36
HREF:  23       Y5:  19
PCLK:  22       Y4:  18
                Y3:  5
                Y2:  4
```

## 📊 Diagrama de Conexiones

```
                     ┌──────────────┐
                     │  ESP32-CAM   │
                     │   WROVER     │
                     └──────┬───────┘
                            │
           ┌────────────────┼────────────────┐
           │                │                │
      ┌────▼────┐      ┌───▼────┐     ┌─────▼─────┐
      │ PIR     │      │  HC-   │     │    LDR    │
      │ HC-SR501│      │  SR04  │     │  GL5528   │
      └─────────┘      └────────┘     └───────────┘
           │                │                │
           └────────────────┴────────────────┘
                            │
                     ┌──────▼───────┐
                     │  Módulo Relé │
                     └──────┬───────┘
                            │
                     ┌──────▼───────┐
                     │     Foco     │
                     │  Iluminación │
                     └──────────────┘
```

## ⚙️ Configuración

### 1. Configuración WiFi

Edita las siguientes líneas en `SmartHome.ino`:

```cpp
const char* ssid = "TU_RED_WIFI";        
const char* password = "TU_CONTRASEÑA";
```

### 2. Configuración Firebase

Crea un proyecto en [Firebase Console](https://console.firebase.google.com/) y obtén:

```cpp
const char* firebaseHost = "https://tu-proyecto.firebaseio.com";
const char* firebaseAuth = "TU_API_KEY";
const char* deviceId = "camara1";  // ID único de tu dispositivo
```

### 3. Configuración de Zona Horaria

Ajusta según tu ubicación:

```cpp
const long gmtOffset_sec = -21600;   // GMT-6 para México Centro
const int daylightOffset_sec = 0;    // Ajuste horario de verano
```

### 4. Configuración de Sensores

Personaliza según tus necesidades:

```cpp
#define PIR_SENSOR_PIN 13           // Pin sensor PIR
#define LED_FLASH_PIN 12            // Pin LED flash
#define TRIG_PIN 14                 // Pin TRIG ultrasónico
#define ECHO_PIN 15                 // Pin ECHO ultrasónico
#define RELAY_PIN 2                 // Pin relé
#define LDR_PIN 33                  // Pin fotorresistencia
#define DISTANCE_THRESHOLD 40        // Distancia en cm (ajustable)
```

### 5. Selección de Modelo de Cámara

En `board_config.h`, descomenta tu modelo:

```cpp
#define CAMERA_MODEL_WROVER_KIT     // Modelo actual
//#define CAMERA_MODEL_AI_THINKER   // Descomenta si usas AI-Thinker
//#define CAMERA_MODEL_M5STACK_PSRAM
// ... otros modelos
```

## 📥 Instalación

### Requisitos Previos

1. **Arduino IDE** (versión 1.8.x o superior) o **PlatformIO**
2. **Soporte ESP32** instalado en Arduino IDE

### Pasos de Instalación

1. **Instalar soporte para ESP32 en Arduino IDE**:
   - Ve a `Archivo` → `Preferencias`
   - En "Gestor de URLs Adicionales de Tarjetas", agrega:
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Ve a `Herramientas` → `Placa` → `Gestor de tarjetas`
   - Busca "ESP32" e instala "esp32 by Espressif Systems"

2. **Instalar bibliotecas requeridas**:
   - Ve a `Programa` → `Incluir Biblioteca` → `Administrar Bibliotecas`
   - Instala:
     - `ESP32` (incluye esp_camera)
     - `WiFi` (incluida con ESP32)
     - `HTTPClient` (incluida con ESP32)
     - `Base64` by Densaugeo

3. **Clonar o descargar este repositorio**:
   ```bash
   git clone https://github.com/JesusLopez28/SmartHome.git
   cd SmartHome
   ```

4. **Configurar la placa en Arduino IDE**:
   - Ve a `Herramientas` → `Placa` → `ESP32 Arduino` → `ESP32 Wrover Module`
   - Configuración recomendada:
     - Upload Speed: `115200`
     - Flash Frequency: `80MHz`
     - Flash Mode: `QIO`
     - Partition Scheme: `Huge APP (3MB No OTA/1MB SPIFFS)`
     - PSRAM: `Enabled`

5. **Subir el código**:
   - Abre `SmartHome.ino`
   - Edita configuraciones (WiFi, Firebase, etc.)
   - Presiona `Ctrl+U` para compilar y subir

### Verificación de Instalación

Abre el Monitor Serie (`Ctrl+Shift+M`) a **115200 baudios**. Deberías ver:

```
╔════════════════════════════════════════╗
║   SMART HOME SECURITY CAMERA v1.0    ║
╚════════════════════════════════════════╝

┌─ Conectando a WiFi
│ .....
└─ ✓ WiFi conectado exitosamente
   IP: 192.168.x.x

┌─ Sincronizando reloj...
└─ ✓ Hora actual: 14:30:25

┌─ Configurando sensores y actuadores:
│  ✓ Sensor PIR configurado (GPIO 13)
│  ✓ Sensor Ultrasónico configurado (TRIG: GPIO 14, ECHO: GPIO 15)
│  ✓ Relé configurado y apagado (GPIO 2)
│  ✓ Fotorresistencia configurada (GPIO 33)
│  ✓ LED Flash configurado (GPIO 12)
└─ Sensores listos

╔════════════════════════════════════════╗
║      SISTEMA INICIADO Y LISTO         ║
╚════════════════════════════════════════╝

📍 Distancia de activación: 40 cm
👁️  Esperando detección de movimiento...
```

## 🚀 Uso

### Funcionamiento Automático

Una vez encendido, el sistema opera de forma autónoma:

1. **Detección de Movimiento**:
   - El sensor PIR detecta movimiento
   - Se activa el LED flash con animación
   - Se captura una foto de alta calidad
   - La imagen se codifica en Base64
   - Se envía a Firebase con timestamp

2. **Control de Iluminación**:
   - La fotorresistencia mide luz ambiente continuamente
   - Si hay oscuridad Y el sensor ultrasónico detecta presencia (< 40 cm)
   - El relé activa la iluminación externa
   - Cuando la persona se aleja, se apaga automáticamente

### Logs del Monitor Serie

**Detección de movimiento:**
```
🚨 ¡MOVIMIENTO DETECTADO!
┌─ Preparando captura:
│  ⚡ Flash: ▓░▓░▓░
│  🔦 Iluminando escena...
│  ⚡ Flash apagado
│  ✓ Imagen: 125478 bytes (1600x1200)
└─ Procesando...

┌─ Codificando imagen:
│  📊 Procesando Base64 ✓
│  📦 Tamaño JSON: 167305 bytes
└─ Listo para enviar

┌─ Enviando a Firebase:
└─ ✓ Respuesta: 200 - Datos enviados correctamente
```

**Control de iluminación:**
```
🌙 Oscuridad detectada
📏 Distancia medida: 25.4 cm
💡 Objeto CERCA - Activando luz
[RELÉ] Encendido → Luz activada

📏 Distancia medida: 52.1 cm
💡 Objeto LEJOS - Desactivando luz
[RELÉ] Apagado → Luz desactivada
```

## 🔥 API Firebase

### Estructura de Datos

#### Colección: `/eventos/fotos`

Cada captura genera un objeto con la siguiente estructura:

```json
{
  "device": "camara1",
  "timestamp": "2025-11-26_14:30:45",
  "type": "motion_detected",
  "width": 1600,
  "height": 1200,
  "size": 125478,
  "image": "data:image/jpeg;base64,/9j/4AAQSkZJRg..."
}
```

#### Colección: `/eventos/sensores`

Registro de eventos de sensores:

```json
{
  "device": "camara1",
  "timestamp": "2025-11-26_14:30:45",
  "sensor": "ultrasonic",
  "event": "relay_on",
  "distance": 25.4
}
```

**Tipos de eventos:**
- `relay_on`: Relé activado
- `relay_off`: Relé desactivado
- `light_detected`: Luz natural detectada
- `darkness_detected`: Oscuridad detectada

### Reglas de Seguridad Firebase (Recomendadas)

```json
{
  "rules": {
    "eventos": {
      ".read": "auth != null",
      ".write": "auth != null",
      "fotos": {
        ".indexOn": ["timestamp", "device"]
      },
      "sensores": {
        ".indexOn": ["timestamp", "device", "sensor"]
      }
    }
  }
}
```

## 🔨 Funciones Principales

### Inicialización

```cpp
bool initCamera()
```
- Configura la cámara ESP32
- Establece resolución UXGA (1600x1200) con PSRAM
- Ajusta parámetros de imagen (brillo, contraste, balance de blancos)
- Retorna `true` si la inicialización fue exitosa

### Captura de Fotos

```cpp
void capturePhotoAndSend()
```
- Activa animación de flash LED
- Captura foto con iluminación
- Codifica en Base64
- Envía a Firebase con metadata

### Medición de Distancia

```cpp
float getDistance()
```
- Envía pulso ultrasónico de 10µs
- Mide tiempo de retorno
- Calcula distancia en centímetros
- Retorna `0` si hay timeout

### Comunicación Firebase

```cpp
void sendToFirebase(String path, String jsonData)
```
- Realiza POST HTTP a Firebase Realtime Database
- Incluye autenticación API
- Maneja respuestas y errores

```cpp
void sendSensorEvent(String sensorType, String event, float value = 0)
```
- Registra eventos de sensores en Firebase
- Incluye timestamp automático
- Valor opcional para datos numéricos

### Utilidades

```cpp
String getTimestamp()
```
- Obtiene hora actual del RTC sincronizado
- Formato: `YYYY-MM-DD_HH:MM:SS`
- Zona horaria GMT-6 (México)

```cpp
void printLocalTime()
```
- Imprime hora actual en Monitor Serie
- Formato: `HH:MM:SS`

## 🔍 Troubleshooting

### Problema: Cámara no inicializa

**Síntomas:**
```
❌ ERROR CRÍTICO: Fallo al inicializar cámara
```

**Soluciones:**
1. Verifica que seleccionaste el modelo correcto en `board_config.h`
2. Asegúrate de tener PSRAM habilitado en `Herramientas` → `PSRAM: Enabled`
3. Revisa las conexiones físicas de la cámara
4. Intenta con una fuente de alimentación de 5V/2A mínimo

### Problema: No conecta a WiFi

**Síntomas:**
```
⚠ ERROR: No se pudo conectar a WiFi
```

**Soluciones:**
1. Verifica SSID y contraseña
2. Asegúrate que la red sea 2.4GHz (ESP32 no soporta 5GHz)
3. Acércate al router
4. Reinicia el router si es necesario

### Problema: Error al enviar a Firebase

**Síntomas:**
```
└─ ✗ Error HTTP: -1
```

**Soluciones:**
1. Verifica la URL de Firebase (debe incluir `https://`)
2. Confirma que la API key sea correcta
3. Revisa las reglas de seguridad de Firebase
4. Asegúrate de tener conexión a Internet

### Problema: Sensor PIR muy sensible

**Síntomas:**
- Múltiples detecciones falsas
- Capturas continuas sin movimiento

**Soluciones:**
1. Ajusta el potenciómetro de sensibilidad en el HC-SR501
2. Aumenta `CAPTURE_INTERVAL` a 5000ms o más
3. Aleja el sensor de fuentes de calor
4. Espera 2 minutos para estabilización tras conectar

### Problema: Relé no activa

**Síntomas:**
- Distancia medida correctamente pero luz no enciende

**Soluciones:**
1. Verifica voltaje del relé (algunos necesitan 5V, no 3.3V)
2. Usa un transistor/MOSFET si el GPIO no proporciona suficiente corriente
3. Revisa conexión: ESP32 → IN, VCC → 5V, GND → GND
4. Prueba activación manual: `digitalWrite(RELAY_PIN, HIGH);`

### Problema: Imágenes oscuras

**Síntomas:**
- Fotos con poca iluminación

**Soluciones:**
1. Verifica que el LED flash esté funcionando
2. Aumenta el tiempo de encendido del flash (actualmente 220ms)
3. Ajusta `led_duty` en `app_httpd.cpp` (max 255)
4. Modifica parámetros de exposición en `initCamera()`:
   ```cpp
   s->set_aec_value(s, 600);  // Aumentar exposición
   s->set_ae_level(s, 2);     // Nivel +2
   ```

## 📝 Notas Importantes

### Seguridad

⚠️ **ADVERTENCIA**: Este código incluye credenciales en texto plano. Para producción:

1. Nunca subas credenciales a repositorios públicos
2. Usa variables de entorno o archivos `.env`
3. Implementa autenticación robusta en Firebase
4. Considera encriptación para datos sensibles

### Consumo de Datos

- Cada foto en Base64 puede ocupar ~170KB
- Considera límites de Firebase Realtime Database
- Plan Spark (gratuito): 1GB descarga/día
- Para uso intensivo, considera Firebase Storage

### Rendimiento

- PSRAM requerido para resolución UXGA
- Sin PSRAM, limitado a SVGA (800x600)
- Codificación Base64 consume tiempo y memoria
- Considera optimizar si necesitas capturas más rápidas

## 🛠️ Personalización

### Cambiar Resolución de Cámara

En `initCamera()`:

```cpp
// Baja resolución (más rápido, menos almacenamiento)
config.frame_size = FRAMESIZE_VGA;  // 640x480

// Alta resolución (más lento, más almacenamiento)
config.frame_size = FRAMESIZE_QXGA; // 2048x1536 (solo con PSRAM)
```

### Ajustar Calidad JPEG

```cpp
config.jpeg_quality = 10;  // 0-63 (menor = mejor calidad, más peso)
```

### Modificar Intervalo de Captura

```cpp
const unsigned long CAPTURE_INTERVAL = 5000; // 5 segundos
```

### Cambiar Umbral de Distancia

```cpp
#define DISTANCE_THRESHOLD 30  // 30 cm en lugar de 40
```

## 📄 Licencia

Este proyecto está licenciado bajo la Licencia Apache 2.0 - ver los archivos fuente para más detalles.

Partes del código base de la cámara © 2015-2016 Espressif Systems (Shanghai) PTE LTD

---

## 👨‍💻 Autor

**Jesús López**
- GitHub: [@JesusLopez28](https://github.com/JesusLopez28)
- Proyecto: SmartHome Security Camera v1.0

---

## 🤝 Contribuciones

¡Las contribuciones son bienvenidas! Si deseas mejorar este proyecto:

1. Fork el repositorio
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add some AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request

---

## 📞 Soporte

Si tienes problemas o preguntas:

1. Revisa la sección de [Troubleshooting](#-troubleshooting)
2. Abre un [Issue](https://github.com/JesusLopez28/SmartHome/issues)
3. Consulta la [documentación de ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)

---

⭐ Si este proyecto te fue útil, considera darle una estrella en GitHub!
