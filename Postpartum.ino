// --- BLYNK CREDENTIALS ---
#define BLYNK_TEMPLATE_ID "TMPL3G_vOza3Z"
#define BLYNK_TEMPLATE_NAME "haemorrhage project"
#define BLYNK_AUTH_TOKEN "rJQUz7sVtQm9BCIqhu3xUNBhyJsxs5Xs"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include "MAX30105.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// --- WiFi Credentials ---
char ssid[] = "vivo V21e 5G";
char pass[] = "chotu2006";

// --- Pin Connections ---
#define ONE_WIRE_BUS 4          // DS18B20 -> D4
#define MOISTURE_PIN 34         // Soil Moisture -> D34
#define BUZZER_PIN 5            // Buzzer -> D5 (Adjusted to your D5 mention)

// --- Objects ---
MAX30105 particleSensor;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- FSM States ---
enum SystemState { STATE_IDLE, STATE_CALIBRATING, STATE_MONITORING, STATE_EMERGENCY };
SystemState currentState = STATE_IDLE;

// --- Thresholds & Variables ---
float baselineSpO2 = 0;
unsigned long stateStartTime = 0;
const int DRY_THRESHOLD = 3200; 
const long IR_PRESENCE_THRESHOLD = 25000; // Threshold to detect finger/blood presence
unsigned long lastBlynkUpdate = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  sensors.begin();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found! Check Wiring.");
    while (1);
  }
  
  // High-sensitivity setup for SpO2
  particleSensor.setup(60, 4, 2, 100, 411, 4096); 
  
  Serial.println("--- HEMORRHAGE WHISPERER START ---");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// Function to calculate SpO2 with Signal Integrity Check
float getCalculatedSpO2() {
  long irValue = particleSensor.getIR();
  
  // If IR is too low, no finger/blood is detected
  if (irValue < IR_PRESENCE_THRESHOLD) {
    return -1.0; 
  }

  long red_sum = 0, ir_sum = 0;
  for (int i = 0; i < 20; i++) {
    red_sum += particleSensor.getRed();
    ir_sum += particleSensor.getIR();
  }
  
  if (ir_sum == 0) return -1.0;

  float R = (float)red_sum / (float)ir_sum;
  
  // Formula adjusted to hit ~96% with a firm finger press
  float spo2 = 112.0 - (16.0 * R); 
  
  if (spo2 > 100) return 100.0;
  if (spo2 < 50) return 50.0;
  return spo2;
}

void loop() {
  Blynk.run();
  
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  int moisture = analogRead(MOISTURE_PIN);
  float currentSpO2 = getCalculatedSpO2();

  // Update Blynk Dashboard every 1 second
  if (millis() - lastBlynkUpdate >= 1000) {
    Blynk.virtualWrite(V1, tempC);
    Blynk.virtualWrite(V2, moisture);
    
    if (currentSpO2 < 0) {
      Blynk.virtualWrite(V3, 0); // Show 0 on Gauge for "No Signal"
    } else {
      Blynk.virtualWrite(V3, currentSpO2);
    }
    lastBlynkUpdate = millis();
  }

  // Finite State Machine
  switch (currentState) {
    
    case STATE_IDLE:
      digitalWrite(BUZZER_PIN, LOW);
      // Logic: Start only if a signal is detected AND moisture triggers (simulating blood)
      if (currentSpO2 > 0 && moisture < DRY_THRESHOLD) {
        Serial.println(">>> SIGNAL DETECTED. Calibrating Baseline...");
        stateStartTime = millis();
        currentState = STATE_CALIBRATING;
      }
      break;

    case STATE_CALIBRATING:
      if (millis() - stateStartTime < 4000) {
        if (currentSpO2 > 0) baselineSpO2 = currentSpO2; 
      } else {
        Serial.print(">>> BASELINE SET AT: "); Serial.println(baselineSpO2);
        currentState = STATE_MONITORING;
      }
      break;

    case STATE_MONITORING:
      // EMERGENCY: Trigger if SpO2 falls 5% below baseline (e.g., 96% -> 91%)
      if (currentSpO2 > 0 && currentSpO2 < (baselineSpO2 - 5.0)) {
        Serial.println(">>> EMERGENCY: SPO2 DECAY DETECTED!");
        Blynk.logEvent("hemorrhage_alert", "EMERGENCY: Significant SpO2 Decay Detected!");
        currentState = STATE_EMERGENCY;
      }
      
      // RESET: If signal is lost (finger removed)
      if (currentSpO2 < 0) {
        Serial.println(">>> SIGNAL LOST. Returning to IDLE.");
        currentState = STATE_IDLE;
      }
      break;

    case STATE_EMERGENCY:
      digitalWrite(BUZZER_PIN, HIGH);
      // To reset, pull moisture sensor out of the cup
      if (moisture > (DRY_THRESHOLD + 200)) {
        digitalWrite(BUZZER_PIN, LOW);
        currentState = STATE_IDLE;
      }
      break;
  }

  // Serial Debugging
  Serial.print("T:"); Serial.print(tempC);
  Serial.print(" | Moist:"); Serial.print(moisture);
  if (currentSpO2 > 0) {
    Serial.print(" | SpO2:"); Serial.print(currentSpO2); Serial.print("%");
  } else {
    Serial.print(" | SpO2: NO SIGNAL");
  }
  Serial.print(" | Base:"); Serial.println(baselineSpO2);

  delay(100); 
}