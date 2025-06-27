mySAS Radiometer Data Processing
================================

This repository includes Python scripts for processing and calibrating data from mySAS, a portable, Arduino-based spectral logging system built around the Adafruit AS7341 sensor. Each script is tailored to one of three radiometric measurements:

Es – Downwelling irradiance
Lt – Water-leaving radiance
Li – Sky radiance

Data from mySAS is calibrated against pySAS, a commercial hyperspectral reference system, using regression-based correction. It uses the L1QBC data as it has more data than L2.

------------------------
Workflow Summary
------------------------

All scripts follow a shared structure:

1. Read and parse logger output (*_LOG.txt)
2. Convert timestamps to actual UTC
3. Correct raw AS7341 readings (normalize by gain × integration time)
4. Trim deployment start/end and remove saturated rows
5. Import matching pySAS data
6. Align times and wavelengths
7. Run wavelength-by-wavelength linear regressions
8. Apply conversion and export summary
9. Generate plots for visual QC


------------------------
Script Differences
------------------------

Script        | Sensor | Measurement Type       | Gain Used | Post-Processing Differences                 
--------------|--------|------------------------|-----------|----------------------------------------------
mySAS_es.py   | Es     | Downwelling irradiance | 16×       | Raw 1 Hz data used directly                  
mySAS_Lt.py   | Lt     | Water-leaving radiance | 16×       | Resampled to 1-min medians to reduce noise   
mySAS_Li.py   | Li     | Sky radiance           | 16×       | Raw 1 Hz data used directly    

------------------------
Output Files
------------------------

Each script generates:
- mySAS_pySAS_<sensor>_regression_summary.csv (slope, intercept, R², RMSE per wavelength)
- PNG plots: time series, spectral snapshots, regression fits

------------------------
Dependencies
------------------------

Requires:
- pandas
- numpy
- matplotlib
- seaborn
- scikit-learn
- scipy

Install via:
pip install pandas numpy matplotlib seaborn scikit-learn scipy

------------------------
Notes
------------------------

- Timestamps are aligned using deployment time offsets
- Gain and integration-time corrections follow AS7341 specs
- pySAS files must be pre-exported to L1BQC CSV format
- Time alignment is based on rounded UTC seconds
- In Arduino code, the LT gain is set to 64x. In this deployment it was 16x. Update if using new data

------------------------
Calibration Equation
------------------------

For each wavelength λ:
Radiance_pySAS(λ) = slope × mySAS_raw(λ) + intercept

