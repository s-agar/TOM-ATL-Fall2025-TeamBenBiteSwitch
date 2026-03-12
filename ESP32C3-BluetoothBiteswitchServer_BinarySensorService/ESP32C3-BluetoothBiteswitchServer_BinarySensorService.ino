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

// See the following for finding UUIDs:
// https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf

#define SERVICE_UUID        "183b" // Binary Sensor Service 0x183B
#define CHARACTERISTIC_UUID "2a06" // Alert Level 0x2A06

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
const int leftClickPin = D0;
const int rightClickPin = D1;
int lastLeftClickState = HIGH;
int lastRightClickState = HIGH;
int16_t clickDataHundredths = 0000;
uint8_t clickData[2];

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

  pinMode(leftClickPin, INPUT_PULLUP);
  pinMode(rightClickPin, INPUT_PULLUP);

  BLEDevice::init("Long name works now");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic =
    pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );

  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Hello World says Biteswitch");
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
    int currentLeftClickState = digitalRead(leftClickPin);
    int currentRightClickState = digitalRead(rightClickPin);
    Serial.print(currentLeftClickState);
    Serial.print(" ");
    Serial.println(currentRightClickState);
    
    if (currentLeftClickState != lastLeftClickState || currentRightClickState != lastRightClickState) {
      clickDataHundredths = 0;
      if (currentLeftClickState == LOW) {
        Serial.println("Left Click LOW.");
        clickDataHundredths += 1;
      }
      if (currentRightClickState == LOW) {
        Serial.println("Right Click LOW.");
        clickDataHundredths += 2;
      }

      clickData[0] = clickDataHundredths & 0xFF;
      clickData[1] = (clickDataHundredths >> 8) & 0xFF;
      pCharacteristic->setValue(clickData, 2);
      pCharacteristic->notify();
      lastLeftClickState = currentLeftClickState;
      lastRightClickState = currentRightClickState;
      delay(10);
    }
  }
}
