# mySAS Spectral Logger

This Arduino sketch powers a lightweight, low-cost ocean optics radiometer using the Adafruit AS7341 10-channel light sensor and an SD-card-enabled Arduino MKR Zero. It logs spectral data with customizable gain and sensor naming for three common measurement configurations:

`Es` – Downwelling irradiance  
`Lt` – Water-leaving radiance  
`Li` – Sky radiance

## Features

- Logs timestamped AS7341 channel data to SD card
- Includes sensor name (`Es`, `Lt`, or `Li`) in each log entry
- Sets appropriate gain based on sensor type
- Lightweight timestamp simulation (no RTC required)
- Logs analog battery voltage for quality control

## How to Use

1. Connect the AS7341 sensor via I2C (STEMMA QT / Qwiic).
2. Insert an SD card formatted as FAT32.
3. Power the Arduino MKR Zero (USB or battery).
4. Open the sketch and set the sensor name near the top of the code:

    const char* SENSOR_NAME = "Lt";  // Options: "Es", "Lt", or "Li"


5. Upload the code. The sketch will:
    - Create a log file named `Lt_log.txt` (or `Es_log.txt`, `Li_log.txt`, depending on the name)
    - Set gain automatically based on the sensor type:
        - `Lt`: Higher gain (e.g., `AS7341_GAIN_64X`)
        - `Es` and `Li`: Default moderate gain (`AS7341_GAIN_16X`)
    - Log every second with output like:

    Time,Sensor,415,445,480,515,555,590,630,680,930,Clear,ASTEP,ATIME,GAIN,Voltage
    2025-06-11 14:17:22,Lt,2567,3868,5607,...,1000,255,5,1.10
 

## Notes

- Timestamps are simulated using `millis()` from a manually defined start time.
- Exposure settings (ATIME and ASTEP) are fixed but can be adjusted manually.
- Analog pin A0 reads battery voltage and logs it for each sample.
- Designed for surface radiometric measurements using multiple fixed-position sensors.

## Dependencies

- [Adafruit AS7341 Library](https://github.com/adafruit/Adafruit_AS7341)
- [Arduino SD Library](https://www.arduino.cc/en/Reference/SD)
- Arduino MKR Zero board package (via Arduino IDE Board Manager)
