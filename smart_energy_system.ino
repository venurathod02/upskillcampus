#include <LiquidCrystal.h>

// --- 1. Hardware Pin Definitions ---
const int currentPin = A0;  // ACS712 sensor
const int voltagePin = A1;  // 100k/10k Voltage divider
const int relayPin = 8;     // Relay control module

// --- 2. Interface Initialization ---
// Initialize the LCD library with standard 4-bit parallel pins: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// --- 3. System Calibration Constants ---
const float vRef = 5.0;           // Standard ADC reference voltage
const float dividerRatio = 11.0;  // Resistor network multiplier ((100k+10k)/10k)
const float sensitivity = 0.185;  // 185mV per Amp for ACS712-05B

void setup() {
  // Initialize Serial telemetry
  Serial.begin(9600);
  
  // Initialize Actuator
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Default state: Load Connected
  
  // Initialize and Boot LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Smart Energy");
  lcd.setCursor(0, 1);
  lcd.print("System Active!");
  
  // FIXED: Removed the invalid line break inside the string literal here
  Serial.println("System Booting... Initialization Complete.");
  
  delay(2000);
  lcd.clear();
}

void loop() {
  // --- Phase 1: Voltage Acquisition ---
  int rawV = analogRead(voltagePin);
  float pinVoltage = (rawV / 1023.0) * vRef;
  float batteryVoltage = pinVoltage * dividerRatio;
  
  // --- Phase 2: Current Acquisition ---
  int rawI = analogRead(currentPin);
  float sensorVoltage = (rawI / 1023.0) * vRef;
  float current = (sensorVoltage - 2.5) / sensitivity; // Offset by quiescent 2.5V
  
  // Software Deadband Filter (Removes noise near 0A)
  if (current < 0.10 && current > -0.10) {
    current = 0.0;
  }
  
  // --- Phase 3: Power Calculation ---
  float power = batteryVoltage * current;
  
  // --- Phase 4: Local Interface Output (LCD) ---
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(batteryVoltage, 1);
  lcd.print("V I:");
  lcd.print(current, 2);
  
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(power, 1);
  lcd.print("W ");
  
  // --- Phase 5: Diagnostic Telemetry Output ---
  Serial.print("Voltage: ");
  Serial.print(batteryVoltage, 2);
  Serial.print(" V | Current: ");
  Serial.print(current, 2);
  Serial.print(" A | Power: ");
  Serial.print(power, 2);
  Serial.println(" W");
  
  // --- Phase 6: Smart Protection Logic ---
  // Thresholds: Battery critical at < 9.0V, Power overload at > 1000.0W
  lcd.setCursor(10, 1);
  if (batteryVoltage < 9.0 || power > 1000.0) {
    digitalWrite(relayPin, LOW); // Critical Fault: Isolate the load
    lcd.print("R:OFF");
  } else {
    digitalWrite(relayPin, HIGH); // Safe: Maintain load connection
    lcd.print("R:ON ");
  }
  
  // System cycle delay to stabilize ADC reads and prevent LCD flicker
  delay(500);
}
