/*
  SD card datalogger

  This example shows how to log data from three analog sensors
  to an SD card using the SD library.

  This example code is in the public domain.

*/

#include <SPI.h>
#include <SD.h>

#include "Adafruit_MAX31855.h"

// Default connection is using software SPI, but comment and uncomment one of
// the two examples below to switch between software SPI and hardware SPI:

// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO 16
#define MAXCS 10
#define MAXCLK 17

// QATCH PCB setup:
#define LED_RED_PIN 24              // Red LED on openQCM PCB (see note below)
#define LED_BLUE_PIN 25             // Blue LED on openQCM PCB (see note below)
#define LED_ORANGE_PIN LED_BUILTIN  // Orange LED on Teensy

bool setup_configured = false;
const int log_interval_ms = 5000;
const int chipSelect = BUILTIN_SDCARD;

String filename = "datalog-0.csv";  // default
char cFilename[16];
unsigned long i = 0;  // increment

// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

void setup() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(LED_ORANGE_PIN, OUTPUT);

  digitalWrite(LED_RED_PIN, HIGH);
  digitalWrite(LED_BLUE_PIN, HIGH);
  digitalWrite(LED_ORANGE_PIN, HIGH);

  // open serial communications and wait for port to open:
  Serial.begin(115200);

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    digitalWrite(LED_BLUE_PIN, LOW);
    digitalWrite(LED_ORANGE_PIN, LOW);
    return;
  }
  Serial.println("card initialized.");

  // find next file name for use this session:
  do {
    filename = "datalog-" + String(i) + ".csv";
    filename.toCharArray(cFilename, filename.length() + 1);
    i++;
  } while (SD.exists(cFilename));
  i = 0;  // reset
  Serial.print("Filename = ");
  Serial.println(filename);

  // wait for MAX chip to stabilize
  delay(500);
  Serial.print("Initializing sensor...");
  if (!thermocouple.begin()) {
    Serial.println("ERROR.");
    digitalWrite(LED_BLUE_PIN, LOW);
    digitalWrite(LED_ORANGE_PIN, LOW);
    return;
  }

  // OPTIONAL: Can configure fault checks as desired (default is ALL)
  // Multiple checks can be logically OR'd together.
  // thermocouple.setFaultChecks(MAX31855_FAULT_OPEN | MAX31855_FAULT_SHORT_VCC);  // short to GND fault is ignored

  Serial.println("Logging to SD...");
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_ORANGE_PIN, LOW);
  setup_configured = true;
}

void loop() {
  // do nothing, if startup error:
  if (!setup_configured) return;

  bool has_error = false;
  digitalWrite(LED_ORANGE_PIN, HIGH);

  // make a string for assembling the data to log:
  String dataString = String(i) + ",";

  double c = thermocouple.readCelsius();
  if (isnan(c)) {
    has_error = true;
    dataString += "FAULT";
    Serial.println("Thermocouple fault(s) detected!");
    uint8_t e = thermocouple.readError();
    if (e & MAX31855_FAULT_OPEN)
      Serial.println("FAULT: Thermocouple is open - no connections.");
    if (e & MAX31855_FAULT_SHORT_GND)
      Serial.println("FAULT: Thermocouple is short-circuited to GND.");
    if (e & MAX31855_FAULT_SHORT_VCC)
      Serial.println("FAULT: Thermocouple is short-circuited to VCC.");
  } else {
    Serial.print("C = ");
    Serial.println(c);
    dataString += String(c);
  }

  // // read three sensors and append to the string:
  // for (int analogPin = 0; analogPin < 3; analogPin++) {
  //   int sensor = analogRead(analogPin);
  //   dataString += String(sensor);
  //   if (analogPin < 2) { dataString += ","; }
  // }

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(cFilename, FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    has_error = true;
    Serial.print("error opening file ");
    Serial.println(filename);
  }

  digitalWrite(LED_ORANGE_PIN, has_error ? HIGH : LOW);
  digitalWrite(LED_RED_PIN, has_error ? HIGH : LOW);

  // wait for interval:
  delay(log_interval_ms);
  
  i++; // next sample for log
}
