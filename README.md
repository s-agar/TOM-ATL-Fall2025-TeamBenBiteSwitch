# Introduction

Welcome to the code repository for Ben's Joystick and Bite Switch! This [Tikkun Olam Makers](https://tomglobal.org/) project was created by students at Georgia Tech in order to help Ben Oxley, an individual with cerebral palsy to more easily control his computer using a customized joystick and bite switch interface. You can find more information about the project [here](todo).

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

# Troubleshooting

## Joystick Drift
**Symptom:** The joystick cursor moves on its own without user input.

**Solution:** Recalibrate the center values in the joystick firmware.

1. Open `ESP32C3-BluetoothJoystickServer_AutomationIOService.ino`
2. With the joystick at rest (centered), read the raw X and Y values from the Serial Monitor
3. Update the `centerX` and `centerY` variables at the top of the file with these values:
   ```cpp
   int centerX = <your_calibrated_X_value>;
   int centerY = <your_calibrated_Y_value>;
   ```
4. Re-upload the firmware to the joystick ESP32C3 board

## Joystick Directions are Wrong
**Symptom:** Joystick movements don't correspond to expected directions (e.g., pushing up moves the cursor down, or pushing left moves it up).

**Solution:** Either switch the X and Y pin assignments or flip the joystick axis values.

**Option 1 - Swap X and Y Pins (if up is down and left is right):**
1. Open `ESP32C3-BluetoothJoystickServer_AutomationIOService.ino`
2. Swap the pin assignments:
   ```cpp
   const int inputXPin = A0;  // Was A2
   const int inputYPin = A2;  // Was A0
   ```
3. Re-upload the firmware

**Option 2 - Flip Axis Values (if movement is rotated 90 degrees):**
1. Open `ESP32C3-BluetoothJoystickServer_AutomationIOService.ino`
2. For the Y-axis, the line `int currentY = 4096 - analogRead(inputYPin);` already inverts the Y value. To flip the X-axis instead, change this line to:
   ```cpp
   int currentX = 4096 - analogRead(inputXPin);
   int currentY = analogRead(inputYPin);
   ```
   Or adjust as needed based on which axes need to be flipped
3. Re-upload the firmware

## Left Switch Corresponds to Right Click and Vice Versa
**Symptom:** The left bite switch triggers a right click and the right bite switch triggers a left click.

**Solution:** Flip the left and right pin assignments.

1. Open `ESP32C3-BluetoothBiteswitchServer_BinarySensorService.ino`
2. Swap the pin assignments:
   ```cpp
   const int leftClickPin = D1;   // Was D0
   const int rightClickPin = D0;  // Was D1
   ```
3. Re-upload the firmware to the bite switch ESP32C3 board
