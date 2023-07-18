# MVV Display with ESP32

This project aims to build a display to see all the connections from the Munich transport system (MVV) at home. The first version will be with an inexpensive LCD display and a ESP32.

## Hardware 
- ESP32 Dev Board (I use the nodemcu-32s board)
  - (Later I plan to use the FireBeetle ESP32 IOT Microcontroller(Supports Wi-Fi &Bluetooth) because of it's low power consumption. To read more about ESP32 Power optimization: https://diyi0t.com/reduce-the-esp32-power-consumption/
- LCD Display (I had at first a 1602 Display, which is not very convenient because of it's size)

## MVG API
Luckily, the MVV has an API where we can get the connections. Unluckily, there is no documentation for this API.

## Referenes and Inspiration
Inspired and took some code from:
- https://github.com/florianlederer/mvv-display-for-ESP32
- https://github.com/mondbaron/mvg/tree/main
- https://github.com/leftshift/python_mvg_api/tree/master
