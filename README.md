# ESP32 Crowd Counter

A passive crowd-density sensor. It runs on an **ESP32** and counts the Wi-Fi and Bluetooth devices nearby. A Python script on a PC reads the counts over USB serial and logs them to a CSV file.

## How it works

Each cycle takes about 30 seconds:

1. **Wi-Fi phase (15 s):** The ESP32 puts its radio in promiscuous mode and sniffs management frames (probe requests). It hops across channels 1 to 13 every 300 ms. Each device is identified by a hash of its MAC address, and only devices with RSSI of -85 dBm or stronger are counted.
2. **Bluetooth phase (15 s):** The ESP32 runs a passive BLE scan. Devices are de-duplicated the same way.
3. **Output:** The ESP32 prints one line over serial at 115200 baud, then starts the next cycle.
4. **Logging:** `logger.py` reads those lines, timestamps them, and appends them to `crowd_log.csv`. You can also type a manual head count while it runs.

## Hardware

- An ESP32 development board (any ESP32 with Wi-Fi and Bluetooth, such as an ESP32 DevKit)
- A USB cable
- A Windows PC to run the logger (the default port is `COM3`)

## Files

| File | Purpose |
|------|---------|
| `sketch_nov7b/sketch_nov7b.ino` | ESP32 firmware (Arduino) |
| `logger.py` | PC-side serial logger (Python) |
| `crowd_log.csv` | Output log, created on first run |

## Setup

### 1. Flash the ESP32

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. In **Boards Manager**, install **esp32** by Espressif.
3. Select your board (for example, *ESP32 Dev Module*) and its COM port.
4. Open `sketch_nov7b/sketch_nov7b.ino` and click **Upload**.

The sketch uses only the Wi-Fi and BLE libraries that come with the ESP32 core, so no extra libraries are needed.

### 2. Run the logger

1. Install Python 3.
2. Install pyserial:
   ```bash
   pip install pyserial
   ```
3. Check which COM port the ESP32 is on in Device Manager. If it is not `COM3`, change `PORT` in `logger.py`.
4. Close the Arduino Serial Monitor. Only one program can use the port at a time.
5. Start the logger:
   ```bash
   python logger.py
   ```

## Usage

- The logger prints one line per reading.
- Type a head count and press Enter whenever it changes. Leave it blank if you don't know it.
- Press `Ctrl+C` to stop. The data is already saved.

The CSV columns are:

```
timestamp, wifi_count, avg_rssi, head_count
```

## Configuration

These constants are at the top of `sketch_nov7b.ino`:

| Constant | Default | Meaning |
|----------|---------|---------|
| `MAX_DEVICES` | 1500 | Maximum unique devices tracked per scan type per cycle |
| `WIFI_SCAN_MS` | 15000 | Length of the Wi-Fi phase (ms) |
| `BLE_SCAN_S` | 15 | Length of the BLE phase (s) |
| `HOP_MS` | 300 | Time spent on each Wi-Fi channel (ms) |
| `RSSI_MIN` | -85 | Ignore devices weaker than this (dBm) |

## Known issues

- **Firmware and logger output formats do not match.** The firmware prints four values per line (`wifi_count,ble_count,wifi_rssi,ble_rssi`), but `logger.py` expects two (`count,rssi`). Every line fails to parse, so no rows are written to the CSV. Either update the logger to read four fields or change the firmware to print two.

## Privacy and limitations

- The sensor counts **devices, not people.** One person may carry several devices, and some people carry none.
- Modern phones often randomize their MAC addresses, which lowers the count's accuracy.
- The sketch stores only 32-bit hashes of MAC addresses in memory, not the raw addresses. Hashes are not anonymization, though, so check your local rules on passive device scanning before you deploy this in a public space.
- Wi-Fi channels are sampled for 300 ms each, so some nearby devices will be missed.
