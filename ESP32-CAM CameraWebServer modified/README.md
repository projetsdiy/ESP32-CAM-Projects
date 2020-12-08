# ESP32-CAM firmware with Webserver modified
This is a modified firmware from [Espressif](https://github.com/espressif/esp32-camera) with the following upgrade
* Stream video at IP/stream. Easy to integrate in a smart home server
* Take snapshot at IP_CAM/jpg/image.jpg

Before to download, you need to modify several settings :
* Network ssid
* Network password
* Adjust picture orientation according the position of the ESP32-CAM FLIP_V, MIRROR_H (true or false)
* Ajust image compression IMAGE_COMPRESSION (0 to 63)
* Uncomment our camera
  * CAMERA_MODEL_WROVER_KIT
  * CAMERA_MODEL_ESP_EYE
  * CAMERA_MODEL_M5STACK_PSRAM
  * CAMERA_MODEL_M5STACK_WIDE
  * CAMERA_MODEL_AI_THINKER
* Connect [FTDI](http://s.click.aliexpress.com/e/_d8ldxmV) to ESP32-CAM
  * GND to GND
  * 5V to 5V
  * RX to UOR
  * TX to UOT
  * IO0 to GND
* Press Reset   
* Configure Arduino IDE setting
  * You need to install ESP32 SDK before to be able to build firmware
  * Flash Mode: QIO (by default)
  * Partition Schema : Huge APP (3MB No OTA)
  * Port: choose your FTDI board
* Upload
* Unconnect IO0 and GND
* Reset
* Open Serial Monitor to get IP
* Open favorite browser and go to IP_CAM/stream
* Enjoy !

More information here [in french](https://projetsdiy.fr/esp32-cam-flash-firmware-test-domoticz-jeedom-home-assistant-nextdom-node-red/)

## Firmware pour ESP32-CAM modifié
Ce firmware est une version modifiée du firmware original d'[Espressif](https://github.com/espressif/esp32-camera)
* Le flux vidéo peut être récupéré facilement depuis un serveur domotique à l'adresse IP_Camera/stream
* Un snapshot peut être récupéré à l'adresse IP_Camera/jpg/image.jpg

Avant de téléverser, vous devez modifier plusieurs paramètres 
* SSID du réseau WiFi
* Mot de passe du réseau WiFi
* MOdifier l'orientation de l'image en fonction de la position de la caméra avec les variables FLIP_V, MIRROR_H 
* Ajuster le taux de compression IMAGE_COMPRESSION (0 à 63)
* Décommenter votre caméra
  * CAMERA_MODEL_WROVER_KIT
  * CAMERA_MODEL_ESP_EYE
  * CAMERA_MODEL_M5STACK_PSRAM
  * CAMERA_MODEL_M5STACK_WIDE
  * CAMERA_MODEL_AI_THINKER
* Connecter l'ESP32-CAM à l'adaptateyr [FTDI](http://s.click.aliexpress.com/e/_d8ldxmV)
  * GND sur GND
  * 5V sur 5V
  * RX sur UOR
  * TX sur UOT
  * IO0 sur GND
* Presser Reset   
* Configurer l'Arduino IDE 
  * Le SDK ESP32 SDK doit être installé avant de pouvoir compiler le firmware
  * Flash Mode: QIO (par défaut)
  * Partition Schema : Huge APP (3MB No OTA)
  * Port: Sélectionner votre adaptateur FTDI 
* Téléverser
* Déconnecter le jumper IO0 et GND
* Reset
* Ouvrer le moniteur série pour récupérer l'adresse IP de la caméra
* Ouvrez votre navigateur internet et allez à l'adresse IP_CAM/stream
* Féliciations !

Plus d'information sur le [blog](https://projetsdiy.fr/esp32-cam-flash-firmware-test-domoticz-jeedom-home-assistant-nextdom-node-red/)
