#include <SPI.h>                   // SPI library for SD card communication
#include <SD.h>                    // SD card handling
#include <Adafruit_AS7341.h>      // Library for Adafruit AS7341 spectral sensor
#include <Wire.h>                 // I2C communication library

File myFile;                      // File object for SD writing
Adafruit_AS7341 as7341;          // AS7341 sensor object

const char* SENSOR_NAME = "Li";  // Change to "Lt" or "Li" for other sensors
String filename = String(SENSOR_NAME) + "_log.txt";

// Simulated timestamp start
const byte seconds_start = 0;
const byte minutes_start = 17;
const byte hours_start = 14;
const byte day_start = 11;
const byte month_start = 6;
const byte year_start = 25;

unsigned long startMillis;        // Time since program began

// Setup runs once at boot
void setup() {
  Serial.begin(9600);             // Start serial monitor
  delay(100);                     // Small delay for USB serial connection
  Wire.begin();                   // Initialize I2C bus

  // Initialize the AS7341 sensor
  if (!as7341.begin()) {
    Serial.println("Could not find AS7341");
    while (1) delay(10);          // Stop if sensor is not found
  }

  // Fixed exposure configuration (manual control)
  as7341.setATIME(255);
  as7341.setASTEP(1000);

   // Adjust gain only if sensor is Lt
  if (strcmp(SENSOR_NAME, "Lt") == 0) {
    as7341.setGain(AS7341_GAIN_32X);
  } else {
    as7341.setGain(AS7341_GAIN_16X);  // Default gain for Es and Li
  }

  // Initialize SD card
  if (!SD.begin(SDCARD_SS_PIN)) {
    Serial.println("SD card init failed!");
    while (1);                    // Stop if SD initialization fails
  }

  // Create and write CSV header if file doesn't already exist
  if (!SD.exists(filename)) {
    myFile = SD.open(filename, FILE_WRITE);
    if (myFile) {
      myFile.println("Time,Sensor,415,445,480,515,555,590,630,680,930,Clear,ASTEP,ATIME,GAIN,Voltage");
      myFile.close();
    } else {
      Serial.println("Failed to create " + String(filename));
    }
  }

  // Start internal millisecond timer
  startMillis = millis();
}

// Generate timestamp string based on elapsed time
String getTimestamp() {
  unsigned long elapsed = millis() - startMillis;
  unsigned long totalSeconds = seconds_start + elapsed / 1000;

  int s = totalSeconds % 60;
  int totalMinutes = minutes_start + totalSeconds / 60;
  int m = totalMinutes % 60;
  int totalHours = hours_start + totalMinutes / 60;
  int h = totalHours % 24;

  char buffer[25];
  snprintf(buffer, sizeof(buffer), "20%02d-%02d-%02d %02d:%02d:%02d",
           year_start, month_start, day_start, h, m, s);
  return String(buffer);
}

// Main logging loop
void loop() {
  // Begin log entry with timestamp and sensor label
  String dataString = getTimestamp() + "," + SENSOR_NAME + ",";

  // Measure battery voltage from analog pin A0
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (3.3 / 1023.0);  // Convert to volts (MKR Zero)

  // Read all 10-channel spectral data
  uint16_t readings[12];
  if (!as7341.readAllChannels(readings)) {
    Serial.println("Error reading AS7341");
    return; // Skip this cycle if read fails
  }

  // Channel remapping to align with spectral wavelengths
  int ch[] = {0, 1, 2, 3, 6, 7, 8, 9, 11, 10};
  for (int i = 0; i < 10; i++) {
    dataString += String(readings[ch[i]]) + ",";
  }

  // Append exposure settings and voltage to line
  dataString += String(as7341.getASTEP()) + ",";
  dataString += String(as7341.getATIME()) + ",";
  dataString += String(as7341.getGain()) + ",";
  dataString += String(voltage);

  // Print to serial monitor
  Serial.println(dataString);

  // Append data to file
  myFile = SD.open(filename, FILE_WRITE);
  if (myFile) {
    myFile.println(dataString);
    myFile.close();
  } else {
    Serial.println("Error writing to " + String(filename));
  }

  delay(1000);  // 1-second logging interval
}
