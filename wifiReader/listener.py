# listener.py
import socket

HOST = "0.0.0.0"      # Listen on all interfaces
PORT = 12345          # Must match the port in your Arduino code

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)

print(f"Listening on {HOST}:{PORT}...")
conn, addr = server.accept()
print(f"Connected by {addr}")

with conn:
    while True:
        try:
            data = conn.recv(1024)
            if not data:
                break
            line = data.decode().strip()
            print(line)
        except Exception as e:
            print(f"Error: {e}")
            break
