#include <AccelStepper.h>
#include <Wire.h>
#define AS5600_ADDR 0x36  // I2C address of the AS5600
#include <Arduino_CRC32.h>

//**************************************************
//**************************************************
// EDIT this values for every joint independently
//**************************************************

const int JOINTNUMBER = 2; //  1 ... 9
const int debug = 0;

const int JOINT = JOINTNUMBER-1; // J{x-1} [0..9]
const int INVERTED = JOINT % 2;; 

Arduino_CRC32 crc32;

int addresses[] = {
  8,  // 1
  9,  // 2
  10, // 3
  11, // 4
  12, // 5
  13, // 6
  14, // 7
  15, // 8
  16  // 9
};

float minEncoderPositions[] = {
  1197,  // 1
  1364,  // 2
  939,  // 3
  1250,  // 4
  1759, // 5
  1523,  // 6
  0,   // 7
  0,   // 8
  0    // 9
};

float maxEncoderPositions[] = {
  3492, // 1
  3642, // 2
  3198, // 3
  3514, // 4
  4012, // 5
  3777, // 6
  4000, // 7
  4000, // 8
  4000  // 9
};


volatile bool homed = 0;

//J1 8 341 3032 239 735
const int slaveAddress = addresses[JOINT]; // I2C address of the Nano ESP32/
float minEncoderPosition = minEncoderPositions[JOINT]; 
float maxEncoderPosition = maxEncoderPositions[JOINT];

float minStepperPosition = 0;
float maxStepperPosition = 10000;

float minTarget = 0;
float maxTarget = 1000;
//**************************************************
//**************************************************

float m1;
float m2;
float m3;
float m4;

// Pin Definitions
const int stepPin = 2;       // Step pin connected to the motor driver
const int dirPin = 3;        // Direction pin connected to the motor driver
const int enablePin = 4;     // Enable pin connected to the motor driver
const int alarmPin = 5;      // Alarm pin connected to the motor driver

const int sw0Pin = 6; 
const int sw1Pin = 7; 
const int sw2Pin = 8;
const int sw3Pin = 9;

const int dio0Pin = 10; // Limit switch
const int dio1Pin = 11; // Encoder DIR, leave LOW
const int dio2Pin = 12; // SDA encoder
const int dio3Pin = 13; // SCL encoder

const int aio0Pin = A0;
const int aio1Pin = A1;
const int aio2Pin = A2;
const int aio3Pin = A3;


// Interval in milliseconds (1000ms = 1 second)
const unsigned long interval = 1000;
unsigned long previousMillis = 0;  // Stores the last time the event was triggered

int receiving = 0;

// Define the AccelStepper interface type for a driver (with step and direction pins)
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

bool prevAlarmState = LOW;

float curTarget = 0;
float curPos = 0;
float prevPos = 0;
float prevTarget = 0;

uint16_t position = 0; // p[osition from magnetic encoder

unsigned long lastSpeedUpdateTime = millis();  

volatile bool callibrationStep = 0;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 100;  // 50ms debounce time

unsigned long lastCommandTime = 0;

float maxSpeed = 4000;
float minSpeed = 10;

void setup() {
 
  // Encoder direction:
  pinMode(dio1Pin, OUTPUT);
  if(INVERTED == 0) digitalWrite(dio1Pin, HIGH);
  else digitalWrite(dio1Pin, LOW);

  initWire();
  
  // Begin the second I2C bus (Wire1) with custom pins
  Wire1.begin(dio2Pin, dio3Pin);  // Custom pins for second I2C bus: SDA = GPIO 12, SCL = GPIO 13
  
  Serial.begin(115200);  // Initialize serial communication for debugging

  // Set enable pin as output
  pinMode(enablePin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  
  // Set alarm pin as input with internal pull-up
  pinMode(alarmPin, INPUT_PULLUP);

  // Enable the motor initially
  //digitalWrite(enablePin, LOW);
  enableStepper();


  stepper.setSpeed(0);
  // Set max speed and acceleration
  stepper.setMaxSpeed(maxSpeed); // Adjust max speed as necessary
  stepper.setAcceleration(maxSpeed); // Adjust acceleration as necessary

  if(INVERTED == 0) {
    stepper.setPinsInverted(true, false, false);  // Invert only the direction pin
  }
  else {
    stepper.setPinsInverted(false, false, false);  // Invert only the direction pin
  }
  stepper.setCurrentPosition(0);

  scanI2CDevices();
}

void loop() {

  checkSerial();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the current time as the last triggered time
    previousMillis = currentMillis;

    // Execute the task every second
    Serial.print("Joint ");
    Serial.print(JOINTNUMBER);
    Serial.println(" is alive!");

    if(!receiving) restartWire();
    receiving = false;

  }

  // Read the AS5600 position (just an example, specific AS5600 communication would depend on library or datasheet)
  Wire1.beginTransmission(AS5600_ADDR);
  Wire1.write(0x0C);  // Address of the position register
  Wire1.endTransmission();
  
  Wire1.requestFrom(AS5600_ADDR, 2);
  if (Wire1.available() >= 2) {
    position = Wire1.read() << 8 | Wire1.read();
    curPos = (float)position;
 
    if (debug) printPos();

    // DISABLE the motor if it's outsie the limits.
    /*
    int margin = 50;
    if(curPos > (maxEncoderPosition + margin) || curPos < (minEncoderPosition - margin) ) {
      disableStepper();
    }
    else{
      enableStepper();
    } 
    */

    if(!homed) {
      if(callibrationStep == 0) {
        if(curPos >= minEncoderPosition) {
          stepper.move(-1); 
        } 
        else {
          stepper.setCurrentPosition(0);
          callibrationStep = 1;
        }
      }
      else{
        if(curPos <= maxEncoderPosition) {
          stepper.move(+1);     
        } 
        else {
          maxStepperPosition = stepper.currentPosition();
          homed = 1;
          restartWire();
        }
      }
      
      stepper.run();  
      return;
    }

    unsigned long currentTime = millis();
    
    checkAlarm();

    // If motor is enabled, run the stepper to the target position
    if (digitalRead(enablePin) == LOW) { 
      long dt = currentTime - lastSpeedUpdateTime;
      lastSpeedUpdateTime = millis();
      float delta = curTarget-encoderPosToTargetPos(position);
      stepper.setSpeed(constrain(delta, -maxSpeed,maxSpeed));
      stepper.run();
    }
    
  //  printPos();
    
  }
}

void printPos(){
  if(!debug) return;
  long currentStepperPos = stepper.currentPosition();


  Serial.print("J");
  Serial.print(JOINT+1);

  Serial.print(", callib.step ");
  Serial.print(callibrationStep);

  Serial.print(", target: ");
  Serial.print(curTarget);
  Serial.print(", encoderPos: ");
  Serial.print(position);
  Serial.print(", stepperPos: ");
  Serial.print(currentStepperPos);

  Serial.print(", maxStepperPosition: ");
  Serial.println(maxStepperPosition);
}

void scanI2CDevices() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Scanning for I2C devices on Wire1...");
  for (address = 1; address < 127; address++) {
    Wire1.beginTransmission(address);
    error = Wire1.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found");
  else Serial.println("done");
}


float stepperPosToEncoderPos(float stepperPos){
  float y = map(stepperPos,minStepperPosition,maxStepperPosition,minEncoderPosition, maxEncoderPosition);
  return constrain(y,minEncoderPosition,maxEncoderPosition);
}

float encoderPosToStepperPos(float encoderPos){
  float y = map(encoderPos,minEncoderPosition,maxEncoderPosition,minStepperPosition, minStepperPosition);
  return constrain(y,minStepperPosition,maxStepperPosition);
}

float encoderPosToTargetPos(float encoderPos){
  float y = map(encoderPos,minEncoderPosition,maxEncoderPosition,minTarget, maxTarget);
  return constrain(y,minTarget,maxTarget);
}

float targetPosToStepperPos(float targetPos){
    float y = map(targetPos,minTarget, maxTarget,minStepperPosition,maxStepperPosition);
  return constrain(y,minStepperPosition,maxStepperPosition);
}


void checkAlarm(){
  // Check alarm pin status
  bool alarmState = digitalRead(alarmPin);
  if(alarmState != prevAlarmState){
    if (alarmState == LOW) {
      // If alarm is triggered, disable motor
      digitalWrite(enablePin, HIGH);  // Disable the motor
      Serial.println("Motor Alarm Triggered. Motor Disabled");
    } else {
      // If no alarm, enable motor
      Serial.println("Motor Alarm Deactivated! Motor Enabled.");
      digitalWrite(enablePin, LOW);   // Enable the motor
      Serial.println("Motor Alarm Deactivated. Motor Enabled");
    }
    prevAlarmState = alarmState;
  }
}



void disableStepper(){
  digitalWrite(enablePin, HIGH);
}
void enableStepper(){
  digitalWrite(enablePin, LOW);
}


void initWire(){
 // Start I2C communication
  Wire.begin(slaveAddress);  // Initialize Nano ESP32 as I2C slave
  Wire.setClock(100000); 
  Wire.setTimeout(3000);
  Wire.onReceive(receiveEvent);  // Register receive event handler
  Wire.onRequest(requestEvent);  // Register request event handler
}

void restartWire(){
  Wire.end();
  delay(100);
  initWire();
}

// Function to handle data received from Giga R1
void receiveEvent(int numBytes) {

  Serial.print("Received ");
  Serial.print(numBytes);
  Serial.println(" bytes");
  
  if(!homed) {
    Serial.println("homing");
    return;
  }

  byte receivedData[4];  // Buffer for the float data
  uint32_t receivedChecksum;

  receiving = true;

  if (numBytes >= 8) {
    for (int i = 0; i < 4; i++) {
      receivedData[i] = Wire.read();
    }
    // Read the 4-byte checksum
    for (int i = 0; i < 4; i++) {
      ((byte*)&receivedChecksum)[i] = Wire.read();
    }

    // Calculate CRC32 of the received float data
    uint32_t calculatedChecksum = crc32.calc(receivedData, sizeof(receivedData));

    // Compare checksums
    if (calculatedChecksum == receivedChecksum) {

      // Convert byte array back to float
      float receivedFloat;
      memcpy(&receivedFloat, receivedData, sizeof(receivedFloat));
      
      Serial.print("Received target: ");
      Serial.println(receivedFloat);

      // ignore if didn't change  
      if(curTarget == receivedFloat) return;

      // disable if out of range
      if(receivedFloat > maxTarget || receivedFloat < minTarget) {
        disableStepper();
        return;
      }
      enableStepper();

      curTarget = receivedFloat;

      Serial.println(curTarget); 
    }
    else {
      Serial.println("CRC32 missmatch!!");
    }
  }  
}

// Function to handle data requests from Giga R1
void requestEvent() {
  if(!homed) return;
  //Serial.println("POSITION REQUESTED");
  float p = encoderPosToTargetPos(curPos);
  Wire.write((byte*)&p, sizeof(p)); 
}

// Function to handle data requests from Giga R1
void requestEventCRC() {
  if(!homed) return;
    byte data[4]; 
    memcpy(data, &curPos, sizeof(curPos));
    uint32_t checksum = crc32.calc(data, sizeof(data));
    // Send the 4 bytes of the float
    for (byte b : data) {
      Wire.write(b);
    }    
    Wire.write((byte*)&checksum, 4);
}


void checkSerial(){
  while (Serial.available()) {


    Serial.println("Serial data available"); 

    float parsedValue = Serial.parseFloat();
    Serial.read();

    // ignore if didn't change  
    if(curTarget == parsedValue) return;


    // disable if out of range
    if(parsedValue > maxTarget || parsedValue < minTarget) {
      disableStepper();
      return;
    }
    enableStepper();

    curTarget = parsedValue;

    Serial.println(curTarget); 
  }
}