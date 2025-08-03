#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_AS7341.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GPS.h>

#define TCAADDR 0x70

Adafruit_AS7341 sensor;
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Adafruit_GPS GPS(&Wire);
WiFiClient client;

const char* ssid = "OliviaCG";
const char* password = "LetMeInPlz";
const char* serverIP = "172.20.10.2";
const int serverPort = 12345;

const uint8_t channelOrder[10] = {0, 1, 2, 3, 6, 7, 8, 9, 11, 10};
uint16_t lt[12], li[12], ed[12];
float ax = NAN, ay = NAN, az = NAN;
float lat = NAN, lon = NAN;
char latDir = 'N', lonDir = 'W';
char timeStr[10] = "00:00:00";

uint8_t atime_lt = 0, atime_li = 0, atime_ed = 0;
uint16_t astep_lt = 0, astep_li = 0, astep_ed = 0;
uint16_t gain_lt = 0, gain_li = 0, gain_ed = 0;

unsigned long ledStart = 0;
bool ledEnabled = true;

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
  delay(10);
}

void blinkRed(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(duration);
    digitalWrite(LED_BUILTIN, LOW); delay(duration);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  delay(1000);

  ledStart = millis();  // Start LED timeout clock

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
    blinkRed(3, 100); while (1);
  }

  // AS7341 sensors
  for (uint8_t port = 0; port <= 2; port++) {
    tcaselect(port);
    if (!sensor.begin()) {
      Serial.print("AS7341 fail on port "); Serial.println(port);
      blinkRed(3, 100); while (1);
    }
    sensor.setATIME(255);
    sensor.setASTEP(1000);
    sensor.setGain((port == 0) ? AS7341_GAIN_32X : AS7341_GAIN_16X);
  }

  // Tilt sensor
  tcaselect(3);
  if (!lis.begin(0x18)) {
    Serial.println("Tilt sensor fail");
    blinkRed(3, 100); while (1);
  }
  lis.setRange(LIS3DH_RANGE_4_G);

  // GPS
  tcaselect(4);
  if (!GPS.begin(0x10)) {
    Serial.println("GPS not found");
    blinkRed(3, 100); while (1);
  }
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

void loop() {
  bool gpsFix = false;
  bool allSensorsOK = true;

  // GPS
  tcaselect(4);
  for (uint8_t i = 0; i < 5; i++) GPS.read();
  if (GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA())) {
    if (GPS.fix) {
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", GPS.hour, GPS.minute, GPS.seconds);
      lat = GPS.latitude;
      lon = GPS.longitude;
      latDir = GPS.lat;
      lonDir = GPS.lon;
      gpsFix = true;
    }
  } else {
    strcpy(timeStr, "NaN");
    lat = NAN;
    lon = NAN;
  }

  // Lt
  tcaselect(0);
  if (!sensor.readAllChannels(lt)) { allSensorsOK = false; memset(lt, 0, sizeof(lt)); }
  gain_lt = sensor.getGain(); atime_lt = sensor.getATIME(); astep_lt = sensor.getASTEP();

  // Li
  tcaselect(1);
  if (!sensor.readAllChannels(li)) { allSensorsOK = false; memset(li, 0, sizeof(li)); }
  gain_li = sensor.getGain(); atime_li = sensor.getATIME(); astep_li = sensor.getASTEP();

  // Ed
  tcaselect(2);
  if (!sensor.readAllChannels(ed)) { allSensorsOK = false; memset(ed, 0, sizeof(ed)); }
  gain_ed = sensor.getGain(); atime_ed = sensor.getATIME(); astep_ed = sensor.getASTEP();

  // Tilt
  tcaselect(3);
  lis.read();
  ax = lis.x_g * 9.80665;
  ay = lis.y_g * 9.80665;
  az = lis.z_g * 9.80665;

  // Build row
  String row = "";
  row += timeStr;
  row += gpsFix ? "," + String(lat, 6) + "," + String(lon, 6) : ",NaN,NaN";
  row += "," + String(ax, 2) + "," + String(ay, 2) + "," + String(az, 2);
  for (uint8_t j = 0; j < 10; j++) row += "," + String(lt[channelOrder[j]]);
  row += "," + String(gain_lt) + "," + String(atime_lt) + "," + String(astep_lt);
  for (uint8_t j = 0; j < 10; j++) row += "," + String(li[channelOrder[j]]);
  row += "," + String(gain_li) + "," + String(atime_li) + "," + String(astep_li);
  for (uint8_t j = 0; j < 10; j++) row += "," + String(ed[channelOrder[j]]);
  row += "," + String(gain_ed) + "," + String(atime_ed) + "," + String(astep_ed);

  // Send over WiFi
  if (!client.connected()) {
    if (!client.connect(serverIP, serverPort)) {
      Serial.println("WiFi send failed");
      blinkRed(2, 150);  // Double red blink
      delay(1000);
      return;
    }
  }

  client.println(row);

  // LED feedback (only for first 5 minutes)
  if (ledEnabled && millis() - ledStart < 300000) {
    if (!allSensorsOK) blinkRed(3, 100);         // Sensor fail
    else if (!gpsFix) blinkRed(1, 500);          // No fix
    else blinkRed(1, 50);                        // All OK (short blink)
  }

  delay(1000);
}
