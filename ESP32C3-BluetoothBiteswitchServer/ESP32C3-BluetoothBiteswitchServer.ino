/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // <-- ADD THIS LINE

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
const int sensorPin = D0;
int lastSensorState = HIGH;

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

  pinMode(sensorPin, INPUT_PULLUP);

  BLEDevice::init("Long name works now");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic =
    pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );

  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Hello World says Shourya");
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
    int currentSensorState = digitalRead(sensorPin);
    Serial.println(currentSensorState);
    if (currentSensorState != lastSensorState) {
      if (currentSensorState == LOW) {
        Serial.println("Sensor LOW. Notifying '1'.");
        pCharacteristic->setValue("1");
        pCharacteristic->notify();
      } else {
        Serial.println("Sensor HIGH. Notifying '0'.");
        pCharacteristic->setValue("0");
        pCharacteristic->notify();
      }
      lastSensorState = currentSensorState;
      delay(10);
    }
  }
  delay(100);
}
