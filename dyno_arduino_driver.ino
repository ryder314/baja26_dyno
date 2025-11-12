// ================================
// Dyno Code
// ================================
// Reads two Hall-effect sensors (primary & secondary) on pins 2 and 3
// Reads a load cell w/ a data and clock on pins 5 and 6
// Calculates rotational velocity (RPM) for each sensor
// Scales load cell values to lbs
// Uses interrupts for accurate timing and easy scalability
// ================================
#include "HX711.h"

const int HALL_PRIMARY_PIN   = 3;   // CVT Primary Hall sensor
const int HALL_SECONDARY_PIN = 2;   // CVT Secondary Hall sensor
const int LED_PIN            = 13;  // Built-in LED (optional indicator)
const int LOAD_CELL_DATA     = 5;   // Load Cell Data Pin
const int LOAD_CELL_CLK      = 6;   // Load Cell CLock Pin

HX711 loadCell;
const float LOAD_CELL_SCALE = -42109.1;

// Variables for timing and pulse tracking
volatile unsigned long lastPrimaryTime = 0;
volatile unsigned long lastSecondaryTime = 0;
volatile unsigned long primaryPeriod = 0;
volatile unsigned long secondaryPeriod = 0;

// RPM calculation variables
float primaryRPM = 0.0;
float secondaryRPM = 0.0;
float loadCellValueLbs = 0.0;

// Number of magnets (pulses per revolution)
const int MAGNETS_PER_REV = 1;

// Timing control for serial output
unsigned long lastPrintTime = 0;
unsigned long PRINT_INTERVAL_MS = 0;  // print/log rate in MS -- defined and sent by python file

// ================================
// Interrupt Service Routines
// ================================
void primaryISR() {
  unsigned long now = micros();
  primaryPeriod = now - lastPrimaryTime;
  lastPrimaryTime = now;
}

void secondaryISR() {
  unsigned long now = micros();
  secondaryPeriod = now - lastSecondaryTime;
  lastSecondaryTime = now;
}

// ================================
// Setup
// ================================
void setup() {
  Serial.begin(115200);
  pinMode(HALL_PRIMARY_PIN, INPUT_PULLUP);
  pinMode(HALL_SECONDARY_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Attach interrupts for both sensors
  attachInterrupt(digitalPinToInterrupt(HALL_PRIMARY_PIN), primaryISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(HALL_SECONDARY_PIN), secondaryISR, FALLING);

  Serial.println("time,prim_rpm,sec_rpm,load_lbs");

  loadCell.begin(LOAD_CELL_DATA, LOAD_CELL_CLK);
  loadCell.set_scale(LOAD_CELL_SCALE);
  loadCell.tare();
}

// ================================
// Main Loop
// ================================
void loop() {

  if (!digitalRead(HALL_PRIMARY_PIN)) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  unsigned long currentMillis = millis();

  // Update and print at fixed intervals
  if (PRINT_INTERVAL_MS != 0 && currentMillis - lastPrintTime >= PRINT_INTERVAL_MS) {
    lastPrintTime = currentMillis;

    // Compute RPM from pulse periods
    noInterrupts();
    unsigned long pPeriod = primaryPeriod;
    unsigned long sPeriod = secondaryPeriod;
    interrupts();

    if (pPeriod > 0)
      primaryRPM = (60.0 * 1e6) / (pPeriod * MAGNETS_PER_REV);
    else
      primaryRPM = 0;

    if (sPeriod > 0)
      secondaryRPM = (60.0 * 1e6) / (sPeriod * MAGNETS_PER_REV);
    else
      secondaryRPM = 0;

    if (loadCell.is_ready()) {
        loadCellValueLbs = loadCell.get_units(1);
    }

    // Print results
    Serial.print(currentMillis / 1000.0, 3);
    Serial.print(",");
    Serial.print(primaryRPM, 0);
    Serial.print(",");
    Serial.print(secondaryRPM, 0);
    Serial.print(",");
    Serial.println(loadCellValueLbs, 2);
    Serial.flush();
  } else if (PRINT_INTERVAL_MS == 0 && Serial.available()) {
    PRINT_INTERVAL_MS = Serial.parseInt();
    Serial.print("# Acknowledged polling rate of ");
    Serial.print(PRINT_INTERVAL_MS);
    Serial.println(" ms.");
    Serial.flush();
  }
}
