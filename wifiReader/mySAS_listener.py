import socket
import os
import glob
import csv

HOST = "0.0.0.0"
PORT = 12345

# Auto-generate new filename like wifi_log_01.csv, wifi_log_02.csv, etc.
def get_new_filename(base="wifi_log", ext=".csv"):
    existing = sorted(glob.glob(f"{base}_*.csv"))
    if not existing:
        return f"{base}_01{ext}"
    last = existing[-1]
    num = int(last.split("_")[-1].split(".")[0])
    return f"{base}_{num+1:02d}{ext}"

FILENAME = get_new_filename()

# Column headers
COLUMN_HEADERS = [
    "Time", "Lat", "Lon", "TiltX", "TiltY", "TiltZ"
] + [f"Lt_{i}" for i in range(10)] + ["Gain_Lt", "ATIME_Lt", "ASTEP_Lt"] \
  + [f"Li_{i}" for i in range(10)] + ["Gain_Li", "ATIME_Li", "ASTEP_Li"] \
  + [f"Ed_{i}" for i in range(10)] + ["Gain_Ed", "ATIME_Ed", "ASTEP_Ed"]

# Write headers
with open(FILENAME, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(COLUMN_HEADERS)

# Start server
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(1)

print(f"Logging to: {FILENAME}")
print(f"Listening on {HOST}:{PORT}...")
conn, addr = server.accept()
print(f"Connected by {addr}")

with conn:
    while True:
        try:
            data = conn.recv(1024)
            if not data:
                print("Connection closed.")
                break
            line = data.decode().strip()
            print(line)

            fields = line.split(",")
            if len(fields) == len(COLUMN_HEADERS):  # only write valid lines
                with open(FILENAME, "a", newline="") as f:
                    writer = csv.writer(f)
                    writer.writerow(fields)

        except Exception as e:
            print(f"Error: {e}")
            break
