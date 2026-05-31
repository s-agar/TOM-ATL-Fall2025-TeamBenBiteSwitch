# Introduction

Welcome to the code repository for Ben's Joystick and Bite Switch! This [Tikkun Olam Makers](https://tomglobal.org/) project was created by students at Georgia Tech in order to help Ben Oxley, an individual with cerebral palsy, use his computer more easily. You can find more information about the project [here](todo).

# Preparation

To prepare your ESP32C3 boards, here's a tutorial: [https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/#getting-started).

The Pico 2 W code is based on examples available in the Raspberry Pi Pico Extension for VS Code. This extension is required to build the Pico code; add this extension to VS Code in order to program the Pico. Here's a tutorial on how to do so: https://www.raspberrypi.com/news/get-started-with-raspberry-pi-pico-series-and-vs-code/.

# Uploading Programs

Make sure your MCU boards are plugged in to your computer with a USB data cable (not a power-only cable).

## Computer Adapter
Connect the Receiver MCU board (Raspberry Pi Pico 2 W) to your computer using the Micro USB cable, open VS Code, select the Raspberry Pi Pico 2 W board option, and upload the [adapter firmware](picow_ble_biteswitch_and_joystick_mouse).

## Bite Switches
Connect the bite switch Transmitter MCU Board (Xiao ESP32C3) to your computer via a USB-C Cable, open your IDE (like Arduino IDE or VS Code), select the ESP32C3 board option, and upload the [bite switch control firmware](ESP32C3-BluetoothBiteswitchServer_BinarySensorService).

## Joystick
Connect the joystick Transmitter MCU Board (Xiao ESP32C3) to your computer via a USB-C Cable, open your IDE, select the ESP32C3 board option, and upload the [joystick control firmware](ESP32C3-BluetoothJoystickServer_AutomationIOService).
