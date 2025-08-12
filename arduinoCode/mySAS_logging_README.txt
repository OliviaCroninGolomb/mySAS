mySAS Logger – HUZZAH32 + Adalogger + WiFi Streaming
====================================================

This project logs spectral (AS7341), tilt (LIS3DH), and GPS data. Data are written simultaneously to the SD card and sent over WiFi to a Python TCP listener for live logging.

----------------------------------------------------
Hardware
----------------------------------------------------
- Adafruit HUZZAH32 Feather (ESP32)
- Adalogger FeatherWing (microSD slot)
- Adafruit TCA9548A I²C multiplexer
- 3× Adafruit AS7341 10-channel spectral sensors (Lt, Li, Ed)
- Adafruit LIS3DH accelerometer (tilt)
- Mini GPS PA1010D
- LED indicator (onboard)

----------------------------------------------------
WiFi Setup
----------------------------------------------------
1. Edit WiFi credentials in the Arduino sketch huzzah_sd_wifi.ino:
   const char* ssid       = "YourNetworkName";
   const char* password   = "YourNetworkPassword";
   const char* serverIP   = "xxx.xxx.xxx.xxx";  // IP of the Python listener machine
   const int   serverPort = 12345;              // match Python listener port

2. Use a network that allows direct TCP connections between the HUZZAH32 and your computer.
   - Home networks: fine
   - University networks: often blocked (may need hotspot or router)
   - Eduroam: usually blocked

3. Find your computer’s local IP (for serverIP in the Arduino code):
   - Windows: ipconfig  (look for IPv4 Address)
   - Mac/Linux: ifconfig or ip addr

----------------------------------------------------
Python Listener
----------------------------------------------------
Code is found in the wifiReader folder.

Open a terminal in that folder, and run:
    python mySAS_listener.py

Run the listener BEFORE powering the HUZZAH32.  
The incoming data will be written to that folder.

----------------------------------------------------
Order of Operations
----------------------------------------------------
1. Start Python TCP listener on your computer.
2. Power up HUZZAH32.
3. Device connects to WiFi → sends one row/sec to listener → also logs to SD card.
4. Stop Python listener (Ctrl+C) when finished.
5. Safely remove SD card to access local CSV files.

----------------------------------------------------
SD Card Logging
----------------------------------------------------
- New CSV file per power-up: log001.csv, log002.csv, etc.
- CSV columns:
  Time,Lat,Lon,TiltX,TiltY,TiltZ,Lt_0..Lt_9,G_Lt,ATIME_Lt,ASTEP_Lt,Li_0..Li_9,G_Li,ATIME_Li,ASTEP_Li,Ed_0..Ed_9,G_Ed,ATIME_Ed,ASTEP_Ed

----------------------------------------------------
Dynamic Integration Time
----------------------------------------------------
The logger automatically adjusts AS7341 integration time (ATIME) and step size (ASTEP) for each sensor independently.  
- If channels approach sensor saturation (~90% of max), integration time is reduced.
- If channels are well below target (~30k counts), integration time is increased.
- This maintains high signal quality while preventing overexposure.

----------------------------------------------------
LED Feedback (first 5 minutes after boot)
----------------------------------------------------
- Triple red blink: any sensor initialization failure
- Slow red blink (500 ms): sensors OK, GPS no fix
- Fast red blink (50 ms): all sensors OK, GPS fix acquired
