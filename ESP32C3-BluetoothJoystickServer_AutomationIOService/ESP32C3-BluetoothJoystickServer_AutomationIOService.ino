/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

// #include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // <-- ADD THIS LINE
#include <BLEDevice.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "1815" // Automation IO Service 0x1815
#define CHARACTERISTIC_UUID "2a58" // Analog 0x2A58

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
const int inputXPin = A2;
const int inputYPin = A0;

int lastX = 0;
int lastY = 0;

uint8_t joystickData[4];

int centerX = 2280;
int centerY = 1799;

int scaleAxis(int reading, int center) {
  int var = 0;
  int result = 0;

  // reading = sq(reading / 4096) * 4096;
  var = reading - center;

  if (abs(var) < 17) {
    var = 0;
  }

  result = map(var, 0 - 2048, 4095 - 2048, -7, 7);
  return result;
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected, restarting advertising...");
      // It's important to restart advertising after a disconnect
      BLEDevice::startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  pinMode(inputXPin, INPUT);
  pinMode(inputYPin, INPUT);

  BLEDevice::init("Long name works now");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic =
    pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );

  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Hello World says Joystick");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (deviceConnected) {
    int currentX = analogRead(inputXPin);
    int currentY = 4096 - analogRead(inputYPin);
    Serial.print("X: ");
    Serial.println(currentX);
    Serial.print("Y: ");
    Serial.println(currentY);
    currentX = scaleAxis(currentX, centerX);
    currentY = scaleAxis(currentY, centerY);
    if (currentX != lastX || currentY != lastY) {
      joystickData[0] = currentX & 0xFF; // First byte is first 8 bits of X
      joystickData[1] = (currentX >> 8) & 0xFF; // Second byte is second 8 bits of X
      joystickData[2] = currentY & 0xFF; // Third byte is first 8 bits of Y
      joystickData[3] = (currentY >> 8) & 0xFF; // Fourth byte is second 8 bits of Y
      pCharacteristic->setValue(joystickData, 4);
      pCharacteristic->notify();
      lastX = currentX;
      lastY = currentY;
      delay(10);
    }
  }
}
