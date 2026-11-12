#include <WiFi.h> // esp32 wi-fi access point 
#include <AsyncTCP.h> // asynchronous TCP connection
#include <Wire.h> // I2C communication
#include <ESPAsyncWebServer.h> // webpage host and Websocket

// Replace with your network credentials
const char* ssid = "A_SALT_TANK_1";
const char* password = "123456789";

// Motor 1 - Left
const int M1_R_EN = 32; 
const int M1_L_EN = 33; 
const int M1_RPWM = 25; 
const int M1_LPWM = 26; 

// ALL PINS TBD, GENERAL ASSUMPTION 
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

// adjusting motor speed separately using numerical input from website
int motorPower = 50; 

bool MotorRunning[5] = {false};

// updates specified motor to a speed. positive is forward negative is backward
void setMotor(int id, int speed) {
  speed = constrain(speed, -255, 255);

  int RPWM, LPWM; 

  // we don't have motor pointers without the shield, so manually set the PWM pins depending on the ID
  // id == 1 is left motor, id == 2 is right motor
  if (id == 1) {
    RPWM = M1_RPWM; 
    LPWM = M1_LPWM; 
  } else if (id == 2) {
    RPWM = M2_RPWM; 
    LPWM = M2_LPWM; 
  } else {
    return; 
  }


  // translated code to raw esp32 library code, no longer shield
  // RPWM is run forward, LPWM is run backward, ALWAYS MAKE SURE ONE IS OFF WHEN RUNNING THE OTHER
  // WILL EXPLODE AND DIE IF BOTH ARE RUNNING AT THE SAME TIME. WE DON'T HAVE ENOUGH MOTORS TO BE DOING 
  // ANYMORE BS 
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

void updateDriveMotors() {
  // translate power to speed 
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


//Creates an asyncronious web server, allowing multiple paralell connections
AsyncWebServer server(80); // hosts webpage at esp32 ip address
AsyncWebSocket ws("/ws");


//Notes for HTML
//make sure the buttons are named button1, button2, etc. this is because of the initbutton function refrencing that as an element
// feel free to change just make sure to update here so the code doesnt break.
//originally is sent just either 0 or 1, now going to be sent first num is id, second is state. this caps the max number of ids to 9, but I think that will be fine for our purposes
//will also only have power go up to 99% so that we can just use 2 digits and make the code easier
//so like "150" for motor 1 50%
//if we need to send all the motor statuses at once we can just introduce a for loop that goes through each set of 3 characters in onmessage
//state should now be an integer with value between 0 and 99, representing 0.00 and 0.99 repectively, rather than a string like it used to be

// index_html stores the html data for the website
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Motor Power</h2>
      <input type = "number" id = "motorPower" min = "0" max = "100" value = "50"> 
      <button onclick ="setMotorSpeed()">Apply Speed </button>

      <p id = "speedStatus"> Current power: 50% </p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    // nothing currently 
  }
  function onLoad(event) {
    initWebSocket();
    initKeyboard(); 
  }

  function initButton(num) {
    document.getElementById('button'+num).addEventListener('click', () => toggle(num));
  }

  // allows key presses to be read, specifically just WASD
  function initKeyboard() {

    document.addEventListener('keydown', function(event) {

      let key = event.key.toLowerCase();

      if (['w', 'a', 's', 'd'].includes(key)) {
        event.preventDefault();

        // this is where the message sent to the board is created 
        // collects direction and sends 1 for KEYDOWN
        if (!event.repeat) {
          websocket.send(key.toUpperCase() + ":1");
        }
      }
    });

    document.addEventListener('keyup', function(event) {

      let key = event.key.toLowerCase();
      
      // this is where the message sent to the board is created 
      // collects direction and sends 0 for KEYUP
      if (['w', 'a', 's', 'd'].includes(key)) {
        event.preventDefault();

        websocket.send(key.toUpperCase() + ":0");
      }
    });
  }

  function setMotorSpeed()  {
    let power = Number(document.getElementById('motorPower').value);

    // checking validity of speed 
    if (!Number.isInteger(power) || power < 0 || power > 100) {
      alert("Invalid entry.");
      return; 
    }

    // setting up message format like "P:10" where 10 is the power 
    if (websocket.readyState === WebSocket.OPEN) {
      websocket.send("P:" + power);

      document.getElementById('speedStatus').innerHTML = "Current power: " + power + "%";
    }
  }
  function toggle(num){
    websocket.send('toggle'+num);
  }
</script>
</body>
</html>
)rawliteral";


// reads messages and translates them to motor controller 
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    char message[16];

    if (len >= sizeof(message)) return; 

    memcpy(message, data, len);
    
    // NEED to indicate where the string ends with \0 for atoi (ascii to int)
    message[len] = '\0';

    char key = message[0];

    // check for "P" from message instead of "W", "A", "S", or "D"
    // specific message check for when the motor speed is changed. Format: "P:50" or "P:23"
    // "P" to differentiate from W, A, S, or D and the other two values indicate the specific power
    if (key == 'P') {      
      motorPower = constrain(atoi(message + 2), 0, 100);
      updateDriveMotors(); 
      return; 
    }

    // check index 2 to get 0 or 1, telling us whether keyup or keydown (pressed or not)
    bool pressed = atoi(message + 2);

    // wasd controls 
    switch (key) {
      // sets keyX values to their respective boolean depending on message received from web server 
      // format: "W:1" or "S:0", first key indicating direction, value meaning pressed or not
      case 'W': keyW = pressed; break; 
      case 'A': keyA = pressed; break; 
      case 'S': keyS = pressed; break; 
      case 'D': keyD = pressed; break; 
      default: return; 
    }

    updateDriveMotors(); 
  }
}

// when message received... read message 
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void setup() {
  Serial.begin(115200);

  // Configure motor driver pins
  // left motor 
  pinMode(M1_R_EN, OUTPUT);
  pinMode(M1_L_EN, OUTPUT);
  pinMode(M1_RPWM, OUTPUT);
  pinMode(M1_LPWM, OUTPUT);

  // right motor pins
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

  //Creates wifi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Set up the web server to handle different routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html);
  });  

/*server.on("/2/on", handleGPIO27On);
  server.on("/2/off", handleGPIO27Off);
  */

  // send messages when anything happens 
  ws.onEvent(onEvent);

  // attach websocket handler to server 
  server.addHandler(&ws);

  
  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming client requests
  ws.cleanupClients(); 
}