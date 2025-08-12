#include <Wire.h>

#define TCAADDR 0x70

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  delay(1000);
  Serial.println("Scanning only Port 4...");

  tcaselect(4);  // Lt sensor
  scanPort();
}

void loop() {}

void scanPort() {
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    if (addr == TCAADDR) continue;
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(addr, HEX);
    }
  }
}
