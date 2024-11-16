/**

Notes: 

use a 3.3K pullup resistor between SDA an 5V and another one between SCL and 5V;
Uses only one set or pullup resistors for the entire line, not one per arduino. 


**/


#include <Wire.h>
#include "Arduino.h"
#include "RPC.h"
#include "SerialRPC.h"
#include <DigitalOut.h>
#include <Arduino_CRC32.h>

using namespace rtos;

// Initialize variables
const int NUM_JOINTS = 9;
const int slaveAddresses[NUM_JOINTS] = { 8, 9, 10, 11, 12, 13, 14, 15, 16 };

const int availableJoints[NUM_JOINTS] = {1,1,1, 1,1,1, 0,0,0 };

int loopCounter = 0;

float targets[NUM_JOINTS] = {0.0};
float positions[NUM_JOINTS] = {0.0};

int jointStatus[NUM_JOINTS] = {1};

int i2cStatus[NUM_JOINTS] = {-1};
int i2cPrevStatus[NUM_JOINTS] = {-1};

Arduino_CRC32 crc32;

Thread syncM7Thread;

void setup() {
  Wire.begin();
  Wire.setClock(100000); 
  Wire.setTimeout(3000);

  Serial.begin(115200);
  
  // Wait until the Serial connection is ready
  while (!Serial) {}
  if (!SerialRPC.begin()) {
    Serial.println("RPC initialization failed");
  }

  syncM7Thread.start(syncM7);

}

void updateJoint(int i) {
  if(!availableJoints[i]) return;

  if (targets[i] != positions[i]) {
 
    byte data[4]; 
    memcpy(data, &targets[i], sizeof(targets[i]));

    uint32_t checksum = crc32.calc(data, sizeof(data));

    Wire.beginTransmission(slaveAddresses[i]);

    // Send the 4 bytes of the float
    for (byte b : data) {
      Wire.write(b);
    }
    
    Wire.write((byte*)&checksum, 4);

    i2cStatus[i] = Wire.endTransmission(true); 
    // 0 -> OK
    // 2 -> Not found!
    // 5 -> timeout

  }
}

void fetchPosition(int i) {
  if(!availableJoints[i]) return;
  // Device is responsive, request data
  int bytesReceived = Wire.requestFrom(slaveAddresses[i], 4, true);
  if (bytesReceived >= 4) {
    Wire.readBytes(reinterpret_cast<char*>(&positions[i]), 4);
  }
  // clear the buffer
  while (Wire.available() > 0) {
    Wire.read();  // Read and discard all remaining bytes in the buffer
  }
}

void syncM7(){
  while (true) {
    for (int i = 0; i < NUM_JOINTS; i++) {
      auto response = RPC.call("updateM4data", i, positions[i]);
      float target = response.as<float>();
      
      if (!isnan(target)) {
        targets[i] = target;
      }

      if (i2cStatus[i] != i2cPrevStatus[i]) {
        RPC.call("updateM4status", i, i2cStatus[i]);
        i2cPrevStatus[i] = i2cStatus[i];
      }
    }
  }
}

void loop() {
  bool allConnected = true;
  for (int i = 0; i < NUM_JOINTS; i++) {
    if(!checkJointConnection[i]) {
      allConnected = false;
      break;
    }
  }   
  for (int i = 0; i < NUM_JOINTS; i++) {
    if(availableJoints[i]){
      if (allConnected) updateJoint(i);
      fetchPosition(i);
    } 
  }
 }

bool checkJointConnection(int id) {
  Wire.beginTransmission(slaveAddresses[id]);
  return (Wire.endTransmission() == 0); // Returns 0 if successful
}