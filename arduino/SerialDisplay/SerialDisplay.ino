/*
 * Displays text sent over the serial port (e.g. from the Serial Monitor) on
 * an attached LCD.
 * YWROBOT
 *Compatible with the Arduino IDE 1.0
 *Library version:1.1
 */
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  Serial.begin(19200);
}

void loop()
{
  lcd.setCursor(0,0);

  // when characters arrive over the serial port...
  if (Serial.available()) { 
    // wait a bit for the entire message to arrive
    delay(20);
    // clear the screen
    lcd.clear();
    // read all the available characters
    while (Serial.available() > 0) {
      char receivedChar = Serial.read();
      if (receivedChar == '\t') {    // Check if the received character is a newline
        lcd.setCursor(0, 0); 
      } else if (receivedChar == '\n') {    // Check if the received character is a newline
        lcd.setCursor(0, 1); 
      } else {
        lcd.write(receivedChar);     // If not a newline, write the character to LCD
      }
    }
  }
}
