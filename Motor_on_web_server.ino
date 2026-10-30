#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_MotorShield.h>

// Replace with your network credentials
const char* ssid = "OLIN_ROBOTICS";
const char* password = "R0B0TS-RULE";

// creates state variable
String outputMotor1State = "off";
//String outputMotor2State = "off";

// Create a web server object
WebServer server(80);

// Create the Motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *Motor1 = AFMS.getMotor(1);


// Function to handle turning motor on
void handleMotor1On() {
  outputMotor1State = "on";
  Motor1->run(FORWARD);
  handleRoot();
}

// Function to handle turning Motor off
void handleMotor1Off() {
  outputMotor1State = "off";
  Motor1->run(RELEASE);
  handleRoot();
}


// Function to handle the root URL and show the current states
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<link rel=\"icon\" href=\"data:,\">";
  html += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
  html += ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}";
  html += ".button2 { background-color: #555555; }</style></head>";
  html += "<body><h1>ESP32 Web Server</h1>";

  // Display GPIO 26 controls
  html += "<p> Motor 1 - State " + outputMotor1State + "</p>";
  if (outputMotor1State == "off") {
    html += "<p><a href=\"/1/on\"><button class=\"button\">ON</button></a></p>";
  } else {
    html += "<p><a href=\"/1/off\"><button class=\"button button2\">OFF</button></a></p>";
  }

  /*// Display GPIO 27 controls
  html += "<p>GPIO 27 - State " + output27State + "</p>";
  if (output27State == "off") {
    html += "<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>";
  } else {
    html += "<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>";
  }
*/
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

 /* // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);
*/
  //starts the pins the motor shield uses
  Wire.begin(21, 22);

  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  //set Motor speed
  Motor1->setSpeed(150);

  // Connect to Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up the web server to handle different routes
  server.on("/", handleRoot);
  server.on("/1/on", handleMotor1On);
  server.on("/1/off", handleMotor1Off);
  /*server.on("/2/on", handleGPIO27On);
  server.on("/2/off", handleGPIO27Off);
  */

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}