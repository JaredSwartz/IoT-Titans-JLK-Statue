import serial
import time
from flask import Flask
arduino = serial.Serial(port='COM5', baudrate=9600, timeout=1)
def write_read(x):
    arduino.write(bytes(x+"\n", 'utf-8'))
    time.sleep(0.05)
    print(arduino.read_until())

app = Flask(__name__)

@app.route("/<c>")
def command(c):
    write_read(c)
    return c

app.run(host="0.0.0.0", port=80)
