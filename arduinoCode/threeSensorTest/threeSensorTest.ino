#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GPS.h>

#define TCAADDR 0x70

// Light and tilt sensors
Adafruit_AS7341 sensor;
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// GPS (on default Wire, port 4)
Adafruit_GPS GPS(&Wire);

// Channel mapping for AS7341
const uint8_t channelOrder[10] = {0, 1, 2, 3, 6, 7, 8, 9, 11, 10};

// Tilt vars
float ax = 0, ay = 0, az = 0;

// GPS vars
float lat = 0.0, lon = 0.0;
char latDir = 'N', lonDir = 'W';
char timeStr[10] = "00:00:00";

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
  delay(10);  // Let mux settle
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(1000);

  // --- Light sensors (ports 0, 1, 2) ---
  for (uint8_t port = 0; port <= 2; port++) {
    tcaselect(port);
    if (!sensor.begin()) {
      Serial.print("Sensor not found on port "); Serial.println(port);
    } else {
      Serial.print("Sensor ready on port "); Serial.println(port);
      sensor.setATIME(255);
      sensor.setASTEP(1000);
      if (port == 0) {
        sensor.setGain(AS7341_GAIN_32X);  // Lt gets higher gain
      } else {
        sensor.setGain(AS7341_GAIN_16X);  // Li and Ed
      }
    }
  }

  // --- Tilt (port 3) ---
  tcaselect(3);
  if (!lis.begin(0x18)) {
    Serial.println("LIS3DH not found on port 3.");
    while (1);
  }
  lis.setRange(LIS3DH_RANGE_4_G);

  // --- GPS (port 4) ---
  tcaselect(4);
  GPS.begin(0x10);  // PA1010D default
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_ANTENNA);
}

void loop() {
  uint16_t lt[12], li[12], ed[12];

  // --- Lt ---
  tcaselect(0);
  sensor.readAllChannels(lt);
  uint16_t gain_lt = sensor.getGain();
  uint16_t astep_lt = sensor.getASTEP();
  uint8_t atime_lt = sensor.getATIME();

  // --- Li ---
  tcaselect(1);
  sensor.readAllChannels(li);
  uint16_t gain_li = sensor.getGain();
  uint16_t astep_li = sensor.getASTEP();
  uint8_t atime_li = sensor.getATIME();

  // --- Ed ---
  tcaselect(2);
  sensor.readAllChannels(ed);
  uint16_t gain_ed = sensor.getGain();
  uint16_t astep_ed = sensor.getASTEP();
  uint8_t atime_ed = sensor.getATIME();

  // --- Tilt ---
  tcaselect(3);
  lis.read();
  ax = lis.x_g * 9.80665;
  ay = lis.y_g * 9.80665;
  az = lis.z_g * 9.80665;

  // --- GPS ---
  tcaselect(4);
  for (uint8_t i = 0; i < 10; i++) GPS.read();  // fill buffer
  if (GPS.newNMEAreceived()) {
    if (GPS.parse(GPS.lastNMEA())) {
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", GPS.hour, GPS.minute, GPS.seconds);
      if (GPS.fix) {
        lat = GPS.latitude;
        lon = GPS.longitude;
        latDir = GPS.lat;
        lonDir = GPS.lon;
      }
    }
  }

  // --- Output ---
  Serial.print("Lt,");
  for (uint8_t j = 0; j < 10; j++) {
    Serial.print(lt[channelOrder[j]]); Serial.print(",");
  }
  Serial.print(astep_lt); Serial.print(",");
  Serial.print(atime_lt); Serial.print(",");
  Serial.print(gain_lt); Serial.print(",");

  Serial.print("Li,");
  for (uint8_t j = 0; j < 10; j++) {
    Serial.print(li[channelOrder[j]]); Serial.print(",");
  }
  Serial.print(astep_li); Serial.print(",");
  Serial.print(atime_li); Serial.print(",");
  Serial.print(gain_li); Serial.print(",");

  Serial.print("Ed,");
  for (uint8_t j = 0; j < 10; j++) {
    Serial.print(ed[channelOrder[j]]); Serial.print(",");
  }
  Serial.print(astep_ed); Serial.print(",");
  Serial.print(atime_ed); Serial.print(",");
  Serial.print(gain_ed); Serial.print(",");

  Serial.print("Tilt,");
  Serial.print(ax, 2); Serial.print(",");
  Serial.print(ay, 2); Serial.print(",");
  Serial.print(az, 2); Serial.print(",");

  Serial.print(timeStr); Serial.print(",");
  if (GPS.fix) {
    Serial.print(lat, 6); Serial.print(latDir); Serial.print(",");
    Serial.print(lon, 6); Serial.print(lonDir);
  } else {
    Serial.print("NoFix,NoFix");
  }
  Serial.println();

  delay(1000);
}
