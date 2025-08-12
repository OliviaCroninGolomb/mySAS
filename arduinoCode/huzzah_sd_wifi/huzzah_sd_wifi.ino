#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_AS7341.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GPS.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#define TCAADDR 0x70
#define SD_CS   33   // HUZZAH32 + Adalogger FeatherWing CS

// -------- Sensors & WiFi --------
Adafruit_AS7341 sensor;
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Adafruit_GPS GPS(&Wire);
WiFiClient client;

const char* ssid       = "OliviaCG";
const char* password   = "LetMeInPlz";
const char* serverIP   = "172.20.10.2";
const int   serverPort = 12345;

// -------- AS7341 channel order --------
const uint8_t channelOrder[10] = {0, 1, 2, 3, 6, 7, 8, 9, 11, 10};

// -------- State buffers --------
uint16_t lt[12], li[12], ed[12];
float ax=NAN, ay=NAN, az=NAN;
float lat=NAN, lon=NAN;
char  timeStr[10] = "00:00:00";

// dynamic exposure
uint8_t  atime_lt=255,  atime_li=255,  atime_ed=255;
uint16_t astep_lt=1000, astep_li=1000, astep_ed=1000;
uint16_t gain_lt=0, gain_li=0, gain_ed=0;

unsigned long ledStart=0;
bool ledEnabled=true;

// -------- SD logging --------
File logFile;
char logFilename[24]={0};
uint32_t lineCount=0;

// ===== helpers =====
void tcaselect(uint8_t i){
  if(i>7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1<<i);
  Wire.endTransmission();
  delay(2);
}

void blinkRed(int n,int d){
  for(int i=0;i<n;i++){ digitalWrite(LED_BUILTIN,HIGH); delay(d); digitalWrite(LED_BUILTIN,LOW); delay(d); }
}

void adjustIntegration(uint16_t* ch, uint8_t& atime, uint16_t& astep){
  uint16_t mx=0; for(int i=0;i<10;i++) if(ch[i]>mx) mx=ch[i];
  const uint16_t target=30000, maxCount=65000;
  if(mx>(uint16_t)(maxCount*0.90)){ if(astep>100) astep-=50; else if(atime>50) atime-=10; }
  else if(mx<target){ if(astep<999) astep+=50; else if(atime<255) atime+=10; }
}

void generateLogFilename(){
  for(int i=1;i<1000;i++){
    snprintf(logFilename,sizeof(logFilename),"/log%03d.csv",i);
    if(!SD.exists(logFilename)) break;
  }
}

bool openNewFileAndWriteHeader(){
  generateLogFilename();
  logFile = SD.open(logFilename, FILE_WRITE);  // create new & position at start
  if(!logFile){ Serial.println("Open new log FAIL"); return false; }

  logFile.print(  "Time,Lat,Lon,TiltX,TiltY,TiltZ");
  for(uint8_t j=0;j<10;j++) logFile.print(String(",Lt_")+j);
  logFile.print(",Gain_Lt,ATIME_Lt,ASTEP_Lt");
  for(uint8_t j=0;j<10;j++) logFile.print(String(",Li_")+j);
  logFile.print(",Gain_Li,ATIME_Li,ASTEP_Li");
  for(uint8_t j=0;j<10;j++) logFile.print(String(",Ed_")+j);
  logFile.println(",Gain_Ed,ATIME_Ed,ASTEP_Ed");
  //logFile.flush();
  if (++lineCount % 10 == 0) logFile.flush();

  Serial.print("Logging to "); Serial.println(logFilename);
  return true;
}

inline void appendRowToSD(const String& row){
  if(!logFile){
    Serial.println("logFile lost; reopening append…");
    logFile = SD.open(logFilename, FILE_APPEND);
    if(!logFile){ Serial.println("Reopen FAIL"); return; }
  }
  size_t n = logFile.println(row);
  logFile.flush();                               // flush each line so you SEE rows immediately
  Serial.printf("SD wrote %u bytes (size=%u)\n", (unsigned)n, (unsigned)logFile.size());
  if(n==0) Serial.println("SD write returned 0 bytes");
}

// ===== setup =====
void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  delay(300);
  ledStart = millis();

  // Force HUZZAH32 SPI pins, then mount SD
  SPI.end();
  SPI.begin(5 /*SCK*/, 19 /*MISO*/, 18 /*MOSI*/, SD_CS);
  Serial.println("Mounting SD...");
  if(!SD.begin(SD_CS, SPI, 10000000)){ Serial.println("SD.begin failed"); }
  else{
    Serial.printf("SD type:%u size:%llu MB\n", SD.cardType(), SD.cardSize()/(1024ULL*1024ULL));
    if(!openNewFileAndWriteHeader()) Serial.println("Header write failed");
  }

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int retries=0;
  while(WiFi.status()!=WL_CONNECTED && retries<20){ delay(500); Serial.print("."); retries++; }
  Serial.println(WiFi.status()==WL_CONNECTED ? "\nWiFi connected!" : "\nWiFi failed.");

  // AS7341 sensors (Lt=0, Li=1, Ed=2)
  for(uint8_t port=0; port<=2; port++){
    tcaselect(port);
    if(!sensor.begin()){ Serial.print("AS7341 fail on port "); Serial.println(port); blinkRed(3,100); while(1); }
    sensor.setGain(port==0 ? AS7341_GAIN_32X : AS7341_GAIN_16X);
    sensor.setATIME(255); sensor.setASTEP(1000);
  }

  // LIS3DH tilt
  tcaselect(3);
  if(!lis.begin(0x18)){ Serial.println("Tilt sensor fail"); blinkRed(3,100); while(1); }
  lis.setRange(LIS3DH_RANGE_4_G);

  // GPS
  tcaselect(4);
  if(!GPS.begin(0x10)){ Serial.println("GPS not found"); blinkRed(3,100); while(1); }
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

// ===== loop =====
void loop(){
  bool gpsFix=false, allOK=true;

  // GPS
  tcaselect(4);
  for(uint8_t i=0;i<5;i++) GPS.read();
  if(GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA())){
    if(GPS.fix){
      snprintf(timeStr,sizeof(timeStr),"%02d:%02d:%02d",GPS.hour,GPS.minute,GPS.seconds);
      lat=GPS.latitude; lon=GPS.longitude; gpsFix=true;
    }else{
      strcpy(timeStr,"NaN"); lat=NAN; lon=NAN;
    }
  }

  // Lt (port 0)
  tcaselect(0);
  if(!sensor.readAllChannels(lt)){ allOK=false; memset(lt,0,sizeof(lt)); }
  adjustIntegration(lt, atime_lt, astep_lt); sensor.setATIME(atime_lt); sensor.setASTEP(astep_lt);
  gain_lt = sensor.getGain();

  // Li (port 1)
  tcaselect(1);
  if(!sensor.readAllChannels(li)){ allOK=false; memset(li,0,sizeof(li)); }
  adjustIntegration(li, atime_li, astep_li); sensor.setATIME(atime_li); sensor.setASTEP(astep_li);
  gain_li = sensor.getGain();

  // Ed (port 2)
  tcaselect(2);
  if(!sensor.readAllChannels(ed)){ allOK=false; memset(ed,0,sizeof(ed)); }
  adjustIntegration(ed, atime_ed, astep_ed); sensor.setATIME(atime_ed); sensor.setASTEP(astep_ed);
  gain_ed = sensor.getGain();

  // Tilt (port 3)
  tcaselect(3);
  lis.read(); ax=lis.x_g*9.80665; ay=lis.y_g*9.80665; az=lis.z_g*9.80665;

  // Build CSV row
  String row; row.reserve(512);
  row += timeStr;
  row += gpsFix ? ","+String(lat,6)+","+String(lon,6) : ",NaN,NaN";
  row += ","+String(ax,2)+","+String(ay,2)+","+String(az,2);
  for(uint8_t j=0;j<10;j++) row += ","+String(lt[channelOrder[j]]);
  row += ","+String(gain_lt)+","+String(atime_lt)+","+String(astep_lt);
  for(uint8_t j=0;j<10;j++) row += ","+String(li[channelOrder[j]]);
  row += ","+String(gain_li)+","+String(atime_li)+","+String(astep_li);
  for(uint8_t j=0;j<10;j++) row += ","+String(ed[channelOrder[j]]);
  row += ","+String(gain_ed)+","+String(atime_ed)+","+String(astep_ed);

  // WiFi send
  if(!client.connected()) client.connect(serverIP, serverPort);
  if(client.connected()) client.println(row);

  // SD append (flush each line)
  appendRowToSD(row);

  // LED feedback (first 5 min)
  if(ledEnabled && millis()-ledStart<300000){
    if(!allOK) blinkRed(3,100);
    else if(!gpsFix) blinkRed(1,500);
    else blinkRed(1,50);
  }

  delay(1000);
}
