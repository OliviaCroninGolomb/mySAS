#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_AS7341.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GPS.h>

#define TCAADDR 0x70
#define SD_CS SDCARD_SS_PIN

Adafruit_AS7341 sensor;
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Adafruit_GPS GPS(&Wire);

const uint8_t channelOrder[10] = {0, 1, 2, 3, 6, 7, 8, 9, 11, 10};
uint16_t lt[12], li[12], ed[12];
float ax = NAN, ay = NAN, az = NAN;
float lat = NAN, lon = NAN;
char latDir = 'N', lonDir = 'W';
char timeStr[10] = "00:00:00";
int logCounter = 0;

uint8_t atime_lt = 0, atime_li = 0, atime_ed = 0;
uint16_t astep_lt = 0, astep_li = 0, astep_ed = 0;
uint16_t gain_lt = 0, gain_li = 0, gain_ed = 0;

char logFilename[20];
File logFile;

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
  delay(10);
}

void blinkRed() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(100);
    digitalWrite(LED_BUILTIN, LOW); delay(100);
  }
}
void blinkYellow() {
  digitalWrite(LED_BUILTIN, HIGH); delay(500);
  digitalWrite(LED_BUILTIN, LOW); delay(500);
}
void blinkGreen() {
  digitalWrite(LED_BUILTIN, HIGH); delay(50);
  digitalWrite(LED_BUILTIN, LOW);
}

void generateLogFilename() {
  for (int i = 1; i < 1000; i++) {
    snprintf(logFilename, sizeof(logFilename), "log%03d.csv", i);
    if (!SD.exists(logFilename)) break;
  }
}

void writeHeader() {
  logFile = SD.open(logFilename, FILE_WRITE);
  if (logFile) {
    logFile.print("Time,Lat,Lon,TiltX,TiltY,TiltZ,");
    for (uint8_t j = 0; j < 10; j++) logFile.print("Lt_" + String(j) + ",");
    logFile.print("Gain_Lt,ATIME_Lt,ASTEP_Lt,");
    for (uint8_t j = 0; j < 10; j++) logFile.print("Li_" + String(j) + ",");
    logFile.print("Gain_Li,ATIME_Li,ASTEP_Li,");
    for (uint8_t j = 0; j < 10; j++) logFile.print("Ed_" + String(j) + ",");
    logFile.println("Gain_Ed,ATIME_Ed,ASTEP_Ed");
    logFile.close();
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  delay(1000);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    blinkRed(); while (1);
  }

  generateLogFilename();
  writeHeader();

  for (uint8_t port = 0; port <= 2; port++) {
    tcaselect(port);
    if (!sensor.begin()) {
      Serial.print("AS7341 fail on port "); Serial.println(port);
      blinkRed(); while (1);
    }
    sensor.setATIME(255);
    sensor.setASTEP(1000);
    if (port == 0) sensor.setGain(AS7341_GAIN_32X);  // Lt
    else sensor.setGain(AS7341_GAIN_16X);           // Li, Ed
  }

  tcaselect(3);
  if (!lis.begin(0x18)) {
    Serial.println("Tilt sensor fail");
    blinkRed(); while (1);
  }
  lis.setRange(LIS3DH_RANGE_4_G);

  tcaselect(4);
  GPS.begin(0x10);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

void loop() {
  bool gpsFix = false;
  bool allSensorsOK = true;

  // GPS
  tcaselect(4);
  for (uint8_t i = 0; i < 5; i++) GPS.read();
  if (GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA()) && GPS.fix) {
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", GPS.hour, GPS.minute, GPS.seconds);
    lat = GPS.latitude;
    lon = GPS.longitude;
    latDir = GPS.lat;
    lonDir = GPS.lon;
    gpsFix = true;
  } else {
    strcpy(timeStr, "NaN");
    lat = NAN;
    lon = NAN;
    gpsFix = false;
  }


  // Lt
  tcaselect(0);
  if (!sensor.readAllChannels(lt)) { allSensorsOK = false; for (int i = 0; i < 12; i++) lt[i] = 0; }
  gain_lt = sensor.getGain(); atime_lt = sensor.getATIME(); astep_lt = sensor.getASTEP();

  // Li
  tcaselect(1);
  if (!sensor.readAllChannels(li)) { allSensorsOK = false; for (int i = 0; i < 12; i++) li[i] = 0; }
  gain_li = sensor.getGain(); atime_li = sensor.getATIME(); astep_li = sensor.getASTEP();

  // Ed
  tcaselect(2);
  if (!sensor.readAllChannels(ed)) { allSensorsOK = false; for (int i = 0; i < 12; i++) ed[i] = 0; }
  gain_ed = sensor.getGain(); atime_ed = sensor.getATIME(); astep_ed = sensor.getASTEP();

  // Tilt
  tcaselect(3);
  lis.read();
  ax = lis.x_g * 9.80665;
  ay = lis.y_g * 9.80665;
  az = lis.z_g * 9.80665;

  // Log
  logFile = SD.open(logFilename, FILE_WRITE);
  if (logFile) {
    // Time
    logFile.print(timeStr);

    // Lat/Lon
    if (gpsFix) {
      logFile.print("," + String(lat, 6));
      logFile.print("," + String(lon, 6));
    } else {
      logFile.print(",NaN,NaN");
    }

    // Tilt
    logFile.print("," + String(ax, 2));
    logFile.print("," + String(ay, 2));
    logFile.print("," + String(az, 2));

    // Lt
    for (uint8_t j = 0; j < 10; j++) logFile.print("," + String(lt[channelOrder[j]]));
    logFile.print("," + String(gain_lt));
    logFile.print("," + String(atime_lt));
    logFile.print("," + String(astep_lt));

    // Li
    for (uint8_t j = 0; j < 10; j++) logFile.print("," + String(li[channelOrder[j]]));
    logFile.print("," + String(gain_li));
    logFile.print("," + String(atime_li));
    logFile.print("," + String(astep_li));

    // Ed
    for (uint8_t j = 0; j < 10; j++) logFile.print("," + String(ed[channelOrder[j]]));
    logFile.print("," + String(gain_ed));
    logFile.print("," + String(atime_ed));
    logFile.print("," + String(astep_ed));

    logFile.println();
    logFile.close();
    logFile.flush();

    if (!allSensorsOK) blinkRed();
    else if (!gpsFix) blinkYellow();
    else blinkGreen();
  }


  delay(1000);
}
