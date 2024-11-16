#include "Arduino_GigaDisplay_GFX.h"
#include <Arduino_CRC32.h>
#include "Arduino.h"
#include "RPC.h"
#include "SerialRPC.h"
#include "Arduino_GigaDisplayTouch.h"

using namespace rtos;

Thread displayThread;

Arduino_GigaDisplayTouch touchDetector;

Arduino_CRC32 crc32;
GigaDisplay_GFX display;

#define BLACK 0x0000
#define GREEN 0x07E0
#define RED   0xF800
#define WHITE 0xFFFF
#define DEBUG 0

const int NUM_JOINTS = 9;
const int DATA_BLOCK_SIZE = 36;
const int CHECKSUM_SIZE = 4;
const int TOTAL_MSG_SIZE = 2 + DATA_BLOCK_SIZE + CHECKSUM_SIZE;

const String ENABLED_MSG = "Enabled";
const String DISABLED_MSG = "Disabled";

float targets[NUM_JOINTS] = {0.0};
float calc_targets[NUM_JOINTS] = {0.0};
float prev_calc_targets[NUM_JOINTS] = {-1.0};
float positions[NUM_JOINTS] = {0.0};
float prev_targets[NUM_JOINTS] = {-1.0};
float prev_positions[NUM_JOINTS] = {-1.0};
int i2cStatus[NUM_JOINTS] = {-1};
int i2cPrevStatus[NUM_JOINTS] = {-1};

boolean enable = HIGH;
boolean prev_enable = LOW;

int backgroundColor = BLACK;

String status_msg = "Ready";
String prev_status_msg = "";

int learn_mode = 0;

unsigned long lastUpdateTime = 0;  // Time of the last update
const unsigned long timeout = 2000;  // 2 seconds timeout


void setup() {
  Serial.begin(115200);
  while (!SerialRPC.begin()) {
    if(DEBUG) Serial.println("RPC initialization failed");
    delay(1000);
  }

  if(DEBUG) Serial.println("RPC initialization successful");
  RPC.bind("updateM4data", updateM4data);
  RPC.bind("updateM4status", updateM4status);

  initDisplay();

  touchDetector.onDetect(gigaTouchHandler);

  displayThread.start(mainUpdateDisplay);
}

bool isValidFloat(float value) {
    // Check if the value is not NaN and not infinite
    return !isnan(value) && !isinf(value);
}

float updateM4data(int id, float position) {
  if(isValidFloat(position)) positions[id] = position;
  return targets[id];
}

void updateM4status(int id, int status) {
  i2cStatus[id] = status;
}

void mainUpdateDisplay() {
  while (true) {
    bool display_updated = false;
    for (int i = 0; i < NUM_JOINTS; i++) {
      updateJointDisplay(i);
      display_updated = true;
    }
    if(enable != prev_enable){
      if(enable) updateEnabledDisplay(ENABLED_MSG);
      else  updateEnabledDisplay(DISABLED_MSG);
      prev_enable = enable;
    }
    if (status_msg != prev_status_msg) {
      lastUpdateTime = millis();  // Update the last update time
      updateStatusDisplay(status_msg);
      prev_status_msg = status_msg;
      display_updated = true;
    }
    if (display_updated) {
      delay(10); // Add a small delay to avoid excessive updates
    }
  }
}

void loop() {
  readSerial();
  if(learn_mode) {
    sendDataToBlender();
    learn_mode = 0;
  }
  if (millis() - lastUpdateTime > timeout && status_msg != "") {
    status_msg = "";
  }
}

void sendDataToBlender(){
  status_msg = "LEARNING MODE";
  for (int i = 0; i < 9; i++) {
      Serial.print(positions[i]);
      if (i < 8) {
          Serial.print(",");  // Add comma between values
      }
  }
  Serial.println();  // End the line
  delay(1000);
}

void readSerial() {
  static byte message[TOTAL_MSG_SIZE];
  static int message_index = 0;
  static bool receiving = false;

  while (Serial.available() > 0) {
    byte incomingByte = Serial.read();
    if (!receiving && incomingByte == '<') {
      receiving = true;
      message_index = 0;
      status_msg = "Rceiving";
    }    
    if (receiving) {         
      learn_mode = 0;
      message[message_index++] = incomingByte;
      if(incomingByte == '>'){
        if(message_index == TOTAL_MSG_SIZE) {
          learn_mode = 0;
          processReceivedData(message);           
        } 
        else{
          if(message[1] == '?' && message_index == 3){
              learn_mode = 1;
          }
          else {
            learn_mode = 0;
            status_msg = "Invalid message length";
          }
        }
        receiving = false;
        message_index = 0;
      }
    }
  }
}

void processReceivedData(byte* message) {
  byte data_block[DATA_BLOCK_SIZE];
  memcpy(data_block, message + 1, DATA_BLOCK_SIZE);
  
  unsigned long received_checksum = *((unsigned long*)(message + 1 + DATA_BLOCK_SIZE));
  unsigned long calculated_checksum = calculateCRC32(data_block, DATA_BLOCK_SIZE);

  if (received_checksum == calculated_checksum) {
    float values[NUM_JOINTS];
    memcpy(values, data_block, DATA_BLOCK_SIZE);
    invKin(values, targets);
    calcServos(values, calc_targets);

    status_msg = "OK";
  } else {
    status_msg = "Invalid checksum";
  }
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  return crc32.calc(data, length);
}

void initDisplay() {
  display.begin();
  display.setRotation(3);
  display.fillScreen(backgroundColor);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.print("ROBOT I/O");
}

void updateJointDisplay(int i) {
  int color = GREEN;
  if (targets[i] != positions[i]) {
    color = abs(targets[i] - positions[i]) > 10 ? RED : WHITE;
  }

  if(targets[i] != prev_targets[i]){
    if(targets[i]>=0)
    updateCell(String(round(targets[i])), 1, 2 + i, backgroundColor, WHITE);
    prev_targets[i] = targets[i];
  }

  if(calc_targets[i] != prev_calc_targets[i]){
    if(calc_targets[i]>=0)
    updateCell(String(round(calc_targets[i])), 2, 2 + i, backgroundColor, WHITE);
    prev_calc_targets[i] = calc_targets[i];
  }

  if(positions[i] != prev_positions[i]){
    if(isValidFloat(positions[i]) && positions[i]>=0)
      updateCell(String(round(positions[i])), 3, 2 + i, backgroundColor, color);
      prev_positions[i] = positions[i];
  }

  if(i2cPrevStatus[i] != i2cStatus[i]){
    updateCell(String(i2cStatus[i]), 4, 2 + i, backgroundColor, WHITE);
    i2cPrevStatus[i] = i2cStatus[i];
  }
  
}

void updateStatusDisplay(const String &text) {
  int x = 300, y = 10;
  display.fillRect(x, y, 640, 40, backgroundColor);
  display.setCursor(x, y);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print(text);
}

void updateEnabledDisplay(const String &text) {
  int x = 420, y = 10;
  display.fillRect(x, y, 640, 40, backgroundColor);
  display.setCursor(x, y);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print(text);
}

void updateCell(String text, int col, int row, int bgColor, int fontColor) {
  int x = 10 + (col - 1) * 200;
  int y = 70 + (row - 1) * 40;
  display.fillRect(x, y, 200, 40, bgColor);
  display.setCursor(x, y);
  display.setTextSize(3);
  display.setTextColor(fontColor);

  // TRIM text to max 6 characters
  if (text.length() > 6) text = text.substring(0, 6);
  display.print(text);
}



void gigaTouchHandler(uint8_t contacts, GDTpoint_t* points) {
  if (contacts > 0) {
    // toggle enable
    enable = !enable;
  }

  if(enable){
    backgroundColor = BLACK;
  }
  else{
    backgroundColor = RED;
  }
}
