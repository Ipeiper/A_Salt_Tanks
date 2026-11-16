//File containing the code to be uploaded to the main esp32 board, also known as the brain
// Currently, this handles controlling the motors and recieving signals using serial from the cam

#include <WiFi.h>

// Wifi network hosted by this board
const char* ssid = "A_SALT_TANK_1";
const char* password = "123456789";

// Serial link to the cam
// Wiring: cam GPIO14 (TX) -> UART_RX_PIN here, cam GPIO13 (RX) <- UART_TX_PIN here, and GND <-> GND
const int UART_RX_PIN = 16;
const int UART_TX_PIN = 17;
const long UART_BAUD = 9600;   // must match the cam

// stop motors if cam isn't communicating
const unsigned long CAM_TIMEOUT_MS = 500; 

// ALL PINS TBD, GENERAL ASSUMPTION

// Motor 1 - Left
const int M1_R_EN = 32; 
const int M1_L_EN = 33; 
const int M1_RPWM = 25; 
const int M1_LPWM = 26; 
 
//Motor 2 - Right
const int M2_R_EN = 21; 
const int M2_L_EN = 22; 
const int M2_RPWM = 27; 
const int M2_LPWM = 13; 

// individual key state, need to be global so we don't have to check 
// for the states of the keys being pressed in loop, only when camera
// sends message that the state has changed (pressed or released) 
bool keyW = false; 
bool keyA = false; 
bool keyS = false; 
bool keyD = false; 

//
int motorPower = 20; 

// keep track of when the last valid message was from when the cam arrived
unsigned long lastMsgMs = 0; 

// for future message to be received from camera
char lineBuf[16];
uint8_t lineLen = 0; 

//Function that sets the motor values
void setMotor(int id, int speed) {
  speed = constrain(speed, -255, 255);

  int RPWM, LPWM; 

  if (id == 1) {
    RPWM = M1_RPWM; 
    LPWM = M1_LPWM; 
  } else if (id == 2) {
    RPWM = M2_RPWM; 
    LPWM = M2_LPWM; 
  } else {
    return; 
  }


  if (speed > 0) {
    analogWrite(LPWM, 0);
    analogWrite(RPWM, speed);
  } else if (speed < 0) {
    analogWrite(RPWM, 0);
    analogWrite(LPWM, abs(speed));
  } else {
    analogWrite(RPWM, 0);
    analogWrite(LPWM, 0);
  }
}
//Function that updates the drive motors with their new values, to be run whenever things change
void updateDriveMotors() {
  int speed = map(motorPower, 0, 100, 0, 255);

  int leftSpeed = 0; 
  int rightSpeed = 0;

  if (keyW) {
    Serial.println("up");
    leftSpeed += speed; 
    rightSpeed += speed; 
  }

  if (keyS) {
    Serial.println("down");
    leftSpeed -= speed;
    rightSpeed -= speed; 
  }

  if (keyA) {
    Serial.println("left");
    leftSpeed -= speed; 
    rightSpeed += speed; 
  }

  if (keyD) {
    Serial.println("right");
    leftSpeed += speed; 
    rightSpeed -= speed; 
  }

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  setMotor(1, leftSpeed);
  setMotor(2, rightSpeed);

}

// sets new state and only touches motors if there's been a change 
void applyState(bool w, bool a, bool s, bool d, int power) {
  if (w == keyW && a == keyA && s == keyS && d == keyD && power == motorPower) return; 

  keyW = w; 
  keyA = a; 
  keyS = s; 
  keyD = d; 
  motorPower = power; 

  Serial.printf("W%d A%d S%d D%d power %d\n", keyW, keyA, keyS, keyD, motorPower);
  updateDriveMotors(); 
}

// handle that line from the camera EX: "1000050"
void handleLine(const char* line) {
  //check for valid length 
  if (strlen(line) != 7) return; 

  for (int i = 0; i < 4; i++) {
    if (line[i] != '0' && line[i] != '1') return; 
  }

  for (int i = 4; i < 7; i++) {
    if (line[i] < '0' || line[i] > '9') return; 
  }

  // avoided any invalid values, now set them to power and wasd
  // ASCII subtraction 

  // get power
  int power = (line[4] - '0') * 100 + (line[5] - '0') * 10 + (line[6] - '0'); 
  power = constrain(power, 0, 100);

  lastMsgMs = millis(); 

  // get and apply wasd + apply power 
  applyState(line[0] == '1', line[1] == '1', line[2] == '1', line[3] == '1', power);
}

// process line received from camera 
void readCam() {
  while (Serial2.available()) {
    char c = Serial2.read(); 

    // the end of a line 
    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      handleLine(lineBuf);
      lineLen = 0; 
    } 

    // the start of a line 
    else if( c!= '\r') {
      if (lineLen < sizeof(lineBuf) - 1) {
        lineBuf[lineLen++] = c; 
      } else {
        lineLen = 0; // way too long, ignore line 
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  //Open Serial port for cam & main communication
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  // Configure motor driver pins
  pinMode(M1_R_EN, OUTPUT);
  pinMode(M1_L_EN, OUTPUT);
  pinMode(M1_RPWM, OUTPUT);
  pinMode(M1_LPWM, OUTPUT);

  pinMode(M2_R_EN, OUTPUT);
  pinMode(M2_L_EN, OUTPUT);
  pinMode(M2_RPWM, OUTPUT);
  pinMode(M2_LPWM, OUTPUT);

  // Enable both motor drivers
  digitalWrite(M1_R_EN, HIGH);
  digitalWrite(M1_L_EN, HIGH);

  digitalWrite(M2_R_EN, HIGH);
  digitalWrite(M2_L_EN, HIGH);

  //set Motor speed
  setMotor(1, 0);
  setMotor(2, 0);

  // host wifi so cam can join it 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("");
  Serial.print("WiFi network up. AP IP address: ");
  Serial.println(WiFi.softAPIP());

}
  
void loop(){
  readCam(); 

  // failsafe 
  if (millis() - lastMsgMs > CAM_TIMEOUT_MS) {
    applyState(false, false, false, false, motorPower);
  }

  
}
