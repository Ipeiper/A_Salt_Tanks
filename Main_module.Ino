//File containing the code to be uploaded to the main esp32 board, also known as the brain
// Currently, this handles controlling the motors and recieving signals using serial from the cam


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
// for the states of the keys being pressed in loop, only when webpage
// sends message that the state has changed (pressed or released) 
bool keyW = false; 
bool keyA = false; 
bool keyS = false; 
bool keyD = false; 

//
int motorPower = 50; 
bool MotorRunning[5] = {false};

//maximum ticks per second Rate
int TPS = 100;
int SPT = 1000/TPS;
int start_time;

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



void setup() {
Serial.begin(115200);
//Open Serial port for cam & main communication
Serial2.begin(9600,SERIAL_8N1,16,17);

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

}

void loop(){
  //start for dynamic timing
  start_time = millis();

  //Reads the serial connection
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    keyW=(data[0] == 1 ) ? true: false;
    keyA=(data[1] == 1 ) ? true: false;
    keyS=(data[2] == 1 ) ? true: false;
    keyD=(data[3] == 1 ) ? true: false;
    motorPower=int(data.substring(4,7));
    updateDriveMotors();

  }

  delay ((millis()-start_time<SPT) ? SPT-(millis()-start_time): 0 );
}
