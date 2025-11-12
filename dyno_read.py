import os
import time
from datetime import datetime
import serial
from serial.tools import list_ports

# --- Config ---
POLLING_RATE_MS = 25
PRINT_RATE_MS = 1000
baud_rate = 115200
preferred_port = "COM9"  # will try this first if it exists

# --- Pick a port robustly ---
ARDUINO_VIDPIDS = {
    (0x2341, 0x0043),  # Arduino Uno (old)
    (0x2341, 0x0001),  # Arduino Uno (rev)
    (0x2A03, 0x0043),  # Arduino (other vendor ID)
    (0x1A86, 0x7523),  # CH340
    (0x10C4, 0xEA60),  # Silicon Labs CP210x
    (0x0403, 0x6001),  # FTDI FT232
}

def choose_serial_port(preferred=None):
    ports = list(list_ports.comports())
    if not ports:
        raise RuntimeError("No serial ports detected. Plug the board in and try again.")
    # If preferred exists, use it
    if preferred:
        for p in ports:
            if p.device.upper() == preferred.upper():
                return p.device
    # Otherwise try to match by VID/PID
    for p in ports:
        if p.vid is not None and p.pid is not None and (p.vid, p.pid) in ARDUINO_VIDPIDS:
            return p.device
    # Fallback to the first port
    return ports[0].device

port = choose_serial_port(preferred_port)
print(f"Using serial port: {port}")

# --- Files ---
out_dir = "saved_data"
os.makedirs(out_dir, exist_ok=True)
FILENAME = os.path.join(out_dir, datetime.now().strftime("dyno_data_%d-%m-%Y, %H-%M.csv"))
print("Saving to:", os.path.abspath(FILENAME))   # <<--- add this


successfully_initialized = False
lastTime = lastPrimVel = lastSecVel = lastLoad = 0.0
lastPrint_ms = time.perf_counter() * 1000

def readyToPrint() -> bool:
    global lastPrint_ms
    now = time.perf_counter() * 1000
    if now - lastPrint_ms >= PRINT_RATE_MS:
        lastPrint_ms = now
        return True
    return False

# --- Open serial (with helpful errors) ---
try:
    ser = serial.Serial(port, baud_rate, timeout=POLLING_RATE_MS / 1000.0)
except serial.SerialException as e:
    # Show what ports *are* available to make debugging obvious
    available = ", ".join(p.device for p in list(list_ports.comports())) or "none"
    raise SystemExit(f"Failed to open {port}: {e}\nAvailable ports: {available}\n"
                     "Close other serial apps and confirm the correct port in Device Manager.")

time.sleep(2.0)
ser.reset_input_buffer()
ser.reset_output_buffer()
ser.write(f"{POLLING_RATE_MS}\n".encode("ascii", errors="ignore"))

data_file = open(FILENAME, "w", buffering=1)

try:
    while True:
        if ser.in_waiting > 0:
            raw = ser.readline()
            try:
                data = raw.decode("utf-8").strip()
            except UnicodeDecodeError:
                data = raw.decode("latin-1", errors="ignore").strip()

            if not data:
                continue

            if ("time" in data) or ("#" in data):
                successfully_initialized = True
                data_file.write(data + "\n")
                if readyToPrint():
                    print(data)
                continue

            if successfully_initialized:
                parts = data.split(",")
                if len(parts) < 4:
                    continue
                try:
                    t, prim, sec, load = map(float, parts[:4])
                except ValueError:
                    continue

                # FIXED comparison parentheses
                if (abs(prim - lastPrimVel) < 100) and (abs(sec - lastSecVel) < 100):
                    data_file.write(data + "\n")
                    if readyToPrint():
                        print(data)

                lastTime, lastPrimVel, lastSecVel, lastLoad = t, prim, sec, load

except KeyboardInterrupt:
    print(f"\nExiting. Test duration (last 'time' field): {lastTime:.2f}")
finally:
    try: data_file.close()
    except: pass
    try: ser.close()
    except: pass
