import serial
import time
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
try:
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(line)
except KeyboardInterrupt:
    pass
finally:
    ser.close()