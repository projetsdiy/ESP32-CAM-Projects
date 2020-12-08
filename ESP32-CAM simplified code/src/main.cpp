/* Five ESP32-CAM tips. Simplified version of HTML and C ++ code
   Fixed IP address. AP mode. Image rotation 90 °. Automatic recovery WiFi connection. HTML code storage
  
   5 astuces pour ESP32-CAM. Version simplifiée du code HTML et C++ 
   Adresse IP fixe. Mode AP. Rotation image 90°. Récupération automatique connexion WiFi. stockage du code HTML
  
   Licence : see licence file
 */
#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// Select camera model
// Dé-commentez (uniquement) votre ESP32-CAM
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

/**********************************************************************/
/*                         PARAMETRES WIFI                            */
/**********************************************************************/
const char* ssid = "enter_your_ssid";
const char* password = "enter_your_password";
// Activate AP mode AP (Access point). User need to connect directly to ESP32 wifi network to access video stream
// Active le mode AP (Access point). L'ESP32-CAM n'est pas connectée au WiFi, on se connecte directement sur la caméra
#define AP_MODE false
const char* ap_ssid = "esp32-cam";
const char* ap_password = "12345678"; // Mini. 8 car
/**********************************************************************/
/*                     USE FIXED IP                                   */
/*                     UTILISE UNE IP FIXE                            */
/**********************************************************************/
#define USE_FIXED_IP true
// Set your Static IP address. Do not use existing IP address (other computer, TV box, printer, smartphone)
// L'adresse IP que vous souhaitez attribuer à l'ESP32-CAM. Attention à ne pas utiliser une adresse existante !
IPAddress local_IP(192, 168, 1, 80);
// Set your Gateway IP address
// Adresse du routeur ou de la box internet
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
/**********************************************************************/
/*  PARAMETRE DE REDEMARRAGE SI LE RESEAU WIFI N'EST PAS DISPONIBLE   */
/**********************************************************************/
int wifi_counter = 0;
#define wifi_try 10
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30          /* Time ESP32 will go to sleep (in seconds) */
/**********************************************************************/
/*                             PROTOTYPES                             */
/**********************************************************************/
void restartESP32Cam();
void startCameraServer();
/**********************************************************************/
/*                WEB SERVER + STREAM SERVER                          */
/*                SERVEUR WEB + SERVEUR VIDEO                         */
/**********************************************************************/
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

// Stream sever port number
// Numéro du port du server vidéo
int port_number; 

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    .row {
      display: flex;
    }
    .leftcol {
      flex: 30%;
    }
    .rightcol {
      flex: 70%;
    }
  </style>
</head>
<body>
  <div class="row">
    <div class="leftcol">
      <h2>ESP32-CAM Stream Server</h2>
      <p>
        <h4>Rotate Image</h4> 
        <button onclick="rotateLeft()">&#171;</button>
        <button onclick="rotateRight()">&#187;</button>
      </p>
      <p>Use only HTML + CSS</p>
    </div>
    <div class="rightcol">
      <img id="stream" style="width:400px" src='http://%s/stream'/>
    </div>
  </div>
</body>
<script>
  var deg = 0;
  function rotateLeft() {
    deg -= 90;
    console.log("Rotate image to left");
    document.getElementById("stream").style.transform = 'rotate(' + deg + 'deg)';
  }function rotateRight() {
    deg += 90;
    console.log("Rotate image to right");
    document.getElementById("stream").style.transform = 'rotate(' + deg + 'deg)';
  }
</script>
</html>)=====";
/*********************************************/
/*        GENERATE MJPEG STREAM              */
/*        GENERE LE FLUX VIDEO MJPEG         */
/*********************************************/
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  static int64_t last_frame = 0;
  if (!last_frame) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      // Echec de la capture de camera
      Serial.println("JPEG capture failed"); 
      res = ESP_FAIL;
    } else {
      if (fb->width > 400) {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            // Echec de la compression JPEG
            Serial.println("JPEG compression failed"); 
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      break;
    }
  }
  last_frame = 0;
  return res;
}

/*******************************************************/
/*         HTML PAGE BUILDER                           */
/*         CONSTRUCTEUR DE LA PAGE HTML                */
/*******************************************************/
static esp_err_t web_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "identity");

  int indexhtmlsize = sizeof(index_html);
  char indexpage[indexhtmlsize] = "";
  strcat(indexpage, index_html);
  char streamip[20] = "";
  
  if ( AP_MODE ) {
    // En mode AP (connexion directe à l'ESP32-CAM), l'IP est toujours 192.168.4.1
    // En mode AP (connexion directe à l'ESP32-CAM), l'IP est toujours 192.168.4.1
    sprintf(streamip, "192.168.4.1:%d", port_number);
  } else {
    sprintf(streamip, "%d.%d.%d.%d:%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], port_number);
  }
  // Replace stream url inside HTML page code
  // Remplace l'adresse du flux vidéo dans le code de la page HTML
  sprintf(indexpage, index_html, streamip);
  int pagezize = strlen(indexpage);

  // Return HTML page source code
  // Renvoie le code source de la page HTML 
  return httpd_resp_send(req, (const char *)indexpage, pagezize);
}

/***********************************************************/
/*        START WEB SERVER AND VIDEO STREAM                */
/*        DEMARRE LE SERVEUR WEB ET LE FLUX VIDEO          */
/***********************************************************/
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = web_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  // Démarre le serveur web de l'interface HTML accessible depuis le navigateur internet
  Serial.printf("Web server started on ort: '%d'\n", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  // Démarre le flux vidéo
  Serial.printf("Stream server started on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }

  port_number = config.server_port;
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
  // Configure camera Pins
  // Configure les broches de la caméra
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Init with high specs to pre-allocate larger buffers
  // Utilise toute la mémoire PSRAM disponible pour augmenter la taille du buffer vidéo 
  /* AVAILABLE RESOLUTIONS
     Résolutions disponibles
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_QQVGA2,   // 128x160
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_HQVGA,    // 240x176
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_CIF,      // 400x296
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_XGA,      // 1024x768
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_UXGA,     // 1600x1200
    FRAMESIZE_QXGA,     // 2048*1536
  */
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // 800x600 
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  if ( AP_MODE ) {
    // En mode AP, on se connecte directement au réseau WiFi de l'ESP32 à l'adresse http://192.168.4.1
    // En mode AP, on se connecte directement au réseau WiFi de l'ESP32 à l'adresse http://192.168.4.1
    /* ACCESS POINT PARAMETERS
      ap_ssid (defined earlier): maximum of 63 characters
      ap_password (defined earlier): minimum of 8 characters; set to NULL if you want the access point to be open
      channel: Wi-Fi channel, number between 1 to 13
      ssid_hidden: (0 = broadcast SSID, 1 = hide SSID)
      max_connection: maximum simultaneous connected clients, max. 4
      --------
      ap_ssid (déjà définit): 63 caractères max.
      ap_password (déjà defint): au minimum 8 caractères. NULL pour un accès libre. déconseillé !!!
      channel: canal Wi-Fi, nombre entre 1 et 13
      ssid_hidden: 0 = diffuser le nom du résau, 1 = cache le nom du réseau SSID
      max_connection: nombre maximum de clients connectés simultannément à l'ESP32-CAM. 4 max. 
    */
    WiFi.softAP(ap_ssid, ap_password);
  } else {  
    if ( USE_FIXED_IP ) {
      if(!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
      }
    }
    WiFi.begin(ssid, password);
    while ( WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      wifi_counter++;

      if ( wifi_counter > wifi_try ) {
        restartESP32Cam();
      }
    }
    Serial.println("");
    Serial.println("WiFi connected");
  }

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  // Start web server and MJPEG stream
  // Démarrer le serveur web et le flux vidéo MJPEG
  startCameraServer();

}

void loop() {
  if ( WiFi.status() != WL_CONNECTED ) {
    // We just lost WiFi connexion!
    // On vient de perdre la connexion WiFi
    restartESP32Cam();
  }

  delay(10);
}

// Auto re-connect WiFi network after a moment
// Récupération automatique de la connexion WiFi s'il est impossible de se connecter ou si la connexion est perdue
void restartESP32Cam()
{
  Serial.println("Impossible to connect WiFi network or connexion lost ! I sleep a moment and I retry later, sorry ");
  // Activate ESP32 deep sleep mode 
  // Met l'ESP32 en mode deep-sleep pour ne pas drainer la batterie ou consommer inutilement
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * TIME_TO_SLEEP);
  esp_deep_sleep_start();
}