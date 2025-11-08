#include <WiFi.h>
#include "esp_camera.h" 
#include "board_config.h" 

void startCameraServer();

const char* ssid = "AaronHero"; 
const char* password = "12345678";

// DEPENDE DE LA RED DE CADA PERSONA
// COMENTAR SI SE DESCONOCEN TODOS LOS PARAMETROS 
IPAddress staticIP(192, 168, 137, 200);   // IP ESTATICA DESEADA
IPAddress gateway(192, 168, 137, 1);      // DIRECCIÓN DE TU ROUTER (Gateway)
IPAddress subnet(255, 255, 255,0);        // MASCARA DE SUBRED
IPAddress primaryDNS(8, 8, 8, 8);         // DNS

esp_err_t initCamera() {
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.xclk_freq_hz = 20000000; 
    config.pixel_format = PIXFORMAT_JPEG;
    
    // --- Configuración de Hardware (Mantener) ---
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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    // --- Rendimiento Optimo (¡Cambios Clave!) ---
    config.frame_size = FRAMESIZE_SVGA; // La más baja para máxima fluidez (320x240)
    config.jpeg_quality = 18;           // Alta compresión
    config.fb_count = 2;                // Usar 2 buffers
    
    // Si la placa tiene PSRAM, usamos la memoria grande
    if (psramFound()) {
      config.fb_location = CAMERA_FB_IN_PSRAM;
      config.grab_mode = CAMERA_GRAB_LATEST; // Solo el frame más reciente
      Serial.println("Chi hay we");
    } else {
      config.fb_location = CAMERA_FB_IN_DRAM;
      config.fb_count = 1; // Usar solo un buffer si no hay PSRAM
      Serial.println("No hay we");

    }
    // ----------------------------------------------------

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Error al inicializar la cámara: 0x%x\n", err);
        return err;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    s->set_vflip(s, 1); 

    return ESP_OK;
}

void initWiFi() {
  Serial.println("Configurando IP estática...");
    
  // COMENTAR SI NO SE USARA IP ESTATICA
  // Configurar la IP estática ANTES de la conexión
  if (!WiFi.config(staticIP, gateway, subnet, primaryDNS)) {
    Serial.println("Falló la configuración estática.");
  }
    
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

    Serial.println("\nWiFi conectado!");
    Serial.print("Dirección IP asignada: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    
    pinMode(GPIO_NUM_32, OUTPUT);
    digitalWrite(GPIO_NUM_32, LOW);

    initWiFi();
    
    if (initCamera() != ESP_OK) {
        Serial.println("¡Configuración de la cámara fallida! Deteniendo...");
        return;
    }

    startCameraServer();

    Serial.print("Servidor de Streaming en: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/stream"); 
    Serial.println("Accede a la IP en tu navegador (o script) para ver el video.");
}

void loop() {
    delay(6);
}