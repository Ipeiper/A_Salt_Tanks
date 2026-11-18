//This is the file that will be uploaded to the cam board

#include "esp_camera.h"
#include <WiFi.h>
#include <math.h>
#include <WebSocketsServer.h>

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_M5STACK_PSRAM
#define CAMERA_MODEL_AI_THINKER

const char* ssid = "A_SALT_TANK_1";
const char* password = "123456789";

// Wiring: CAM GPIO14 (TX) -> main board RX, CAM GPIO13 (RX) <- main board TX, and GND <-> GND.
const int UART_RX_PIN = 13;
const int UART_TX_PIN = 14;
const long UART_BAUD = 9600;


// Port 80 = website, port 81 = video stream (both started in app_httpd.cpp),
// so the control WebSocket is on 82.
const uint16_t WS_PORT = 82;
const unsigned long SEND_INTERVAL_MS = 100;   // state is re-sent to the main board this often
const unsigned long CLIENT_TIMEOUT_MS = 500;  // no message from the website for this long -> stop

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void startCameraServer();

WebSocketsServer webSocket = WebSocketsServer(WS_PORT);

// make global key states so we only change them when the website sends a message 
bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;
int motorpower = 20; 


// these variables will be used to check if the website timed out, acting as a fail-safe
// when the website last sent us something
unsigned long lastMsgMs = 0; 

// when we last sent the state to the main board   
unsigned long lastSendMs = 0;   

//Function that sends the current state of the keys to the main board
// the arguements are the true false states of the buttons they are associated with
// uses a communication format of strings with the first 4 characters devoted to the bool state of the keys WASD
// then the next 3 charcters are the motor power from 000-100
void sendButtonStates(bool W, bool A, bool S, bool D, int power){
  
  // constrain power from 0 to 100  
  int p = constrain(power, 0, 100);

  // 7 characters + ending marker of \0
  char out[8];

  // 03d allocating 3 digits for the power value 
  snprintf(out, sizeof(out), "%d%d%d%d%03d", W ? 1 : 0, A ? 1 : 0, S ? 1 : 0, D ? 1 : 0, p);

  Serial2.println(out);
  lastSendMs = millis();
}

//handles the websocket messages on the camera side
void handleWebSocketMessage(uint8_t *data, size_t len) {
  if (len != 7) return; // ignores invalid length data

  // data[x] is a character, not boolean, so 
  bool w = (data[0] == '1');
  bool a = (data[1] == '1');
  bool s = (data[2] == '1');
  bool d = (data[3] == '1');

  // collect power 
  for (int i = 4; i < 7; i++) {
    // checks if the ASCII value is within bounds, ignore if not 
    if (data[i] < '0' || data[i] > '9') return;
  }

  // subtract '0' to convert ASCII to int and add up all values
  int power = (data[4] - '0') * 100 + (data[5] - '0') * 10 + (data[6] - '0');

  // set key states 
  keyW = w; 
  keyA = a; 
  keyS = s; 
  keyD = d; 

  motorpower = constrain(power, 0, 100);
  lastMsgMs = millis(); 

  sendButtonStates(keyW,keyA,keyS,keyD,motorpower);
}

// stop everything and release all keys 
void releaseAllKeys() {
  keyW = keyA = keyS = keyD = false; 
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("WebSocket client %u connected\n", num);
      lastMsgMs = millis();
      break;
    
    case WStype_TEXT: 
      handleWebSocketMessage(payload, length);
      break;

    case WStype_DISCONNECTED:
      Serial.printf("WebSocket client %u disconnected\n", num);
      releaseAllKeys(); 
      sendButtonStates(keyW, keyA, keyS, keyD, motorpower);
      break; 
      
    default: 
      break; 
  }
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  //Open Serial port for cam & main communication
  Serial2.begin(UART_BAUD,SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  // make sure it starts stopped 
  sendButtonStates(false, false, false, false, 0);

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
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(5000);
    ESP.restart();
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  unsigned long startMs = millis(); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startMs > 20000) {
      Serial.println("\nWiFi connect timed out, restarting");
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  startCameraServer();
  // Begins the websocket connection and how to manage it
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  webSocket.loop();

  unsigned long now = millis(); 

  // failsafe (if tabs close of wifi drops)
  if (now - lastMsgMs > CLIENT_TIMEOUT_MS) {
    releaseAllKeys(); 
    sendButtonStates(false, false, false, false, motorpower);
  }

  // failsafe (if network lost)
  if (WiFi.status() != WL_CONNECTED) {
    releaseAllKeys();
    sendButtonStates(false, false, false, false, motorpower);
  }

  // constantly send the states over 
  if (now - lastSendMs >= SEND_INTERVAL_MS) {
    sendButtonStates(keyW, keyA, keyS, keyD, motorpower);
  }
}
