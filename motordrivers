const int RPWM = 25, LPWM = 26, R_EN = 32, L_EN = 33, R_IS = 34, L_IS = 35;
const int PWM_HZ = 20000, PWM_BITS = 8;
const int MAX_DUTY = 127;          // ~50% cap, protects the 12V motor on an 18V pack
const float TRIP_VOLTS = 99;       // trip disabled for testing
const int RAMP_STEP = 3;           // duty change per loop, for soft start

int current = 0;                   // signed duty: + forward, - reverse

void stopMotor() {
  ledcWrite(RPWM, 0); ledcWrite(LPWM, 0);
  digitalWrite(R_EN, LOW); digitalWrite(L_EN, LOW);
  current = 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println("v2 started");
  pinMode(R_EN, OUTPUT); pinMode(L_EN, OUTPUT);
  digitalWrite(R_EN, LOW); digitalWrite(L_EN, LOW);   // disabled until ready
  ledcAttach(RPWM, PWM_HZ, PWM_BITS);
  ledcAttach(LPWM, PWM_HZ, PWM_BITS);
  ledcWrite(RPWM, 0); ledcWrite(LPWM, 0);
  analogSetPinAttenuation(R_IS, ADC_11db);
  analogSetPinAttenuation(L_IS, ADC_11db);
  digitalWrite(R_EN, HIGH); digitalWrite(L_EN, HIGH);
}

void drive(int target) {           // target in -MAX_DUTY..MAX_DUTY
  target = constrain(target, -MAX_DUTY, MAX_DUTY);
  // Stop before reversing so the H-bridge doesn't fight the motor's momentum
  if ((current > 0 && target < 0) || (current < 0 && target > 0)) target = 0;
  if (current < target) current = min(current + RAMP_STEP, target);
  else if (current > target) current = max(current - RAMP_STEP, target);
  ledcWrite(RPWM, current > 0 ? current : 0);
  ledcWrite(LPWM, current < 0 ? -current : 0);
}

void loop() {
  float amps_v = max(analogReadMilliVolts(R_IS), analogReadMilliVolts(L_IS)) / 1000.0;
  if (amps_v > TRIP_VOLTS) {       // overcurrent, e.g. stall
    stopMotor();
    Serial.println("OVERCURRENT, halted");
    while (true) delay(1000);      // needs a reset to restart
  }

  // Demo: forward 3 s, stop 1 s, reverse 3 s, stop 1 s
  static unsigned long t0 = millis();
  unsigned long t = (millis() - t0) % 8000;
  int target = (t < 3000) ? MAX_DUTY : (t < 4000) ? 0 : (t < 7000) ? -MAX_DUTY : 0;
  drive(target);

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.print("target="); Serial.print(target);
    Serial.print(" duty="); Serial.println(current);
  }
  delay(10);
}