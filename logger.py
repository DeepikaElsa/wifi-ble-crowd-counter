import serial, csv, os, threading
from datetime import datetime

PORT = "COM3"
FILE = "crowd_log.csv"

ser = serial.Serial(PORT, 115200, timeout=1)
head_count = ""   # stays blank until you type a number

def read_input():
    global head_count
    while True:
        head_count = input().strip()

print("Logging started. Type the head count and press Enter whenever it changes.")
print("Leave it blank if you don't know. Press Ctrl+C to stop.\n")
threading.Thread(target=read_input, daemon=True).start()

new_file = not os.path.exists(FILE)
with open(FILE, "a", newline="") as f:
    w = csv.writer(f)
    if new_file:
        w.writerow(["timestamp", "wifi_count", "avg_rssi", "head_count"])
    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            try:
                count, rssi = [int(x) for x in line.split(",")]
            except ValueError:
                continue
            ts = datetime.now().strftime("%H:%M:%S")
            w.writerow([datetime.now().isoformat(timespec="seconds"), count, rssi, head_count])
            f.flush()
            print(f"[{ts}] devices={count}  rssi={rssi}  head_count={head_count or '-'}")
    except KeyboardInterrupt:
        print("\nStopped. Data saved to", FILE)