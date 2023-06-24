#include <Wire.h>

#define TM1650_ADDRESS 0x24

#define LED_ADDRESS 0x34
#define DATA_ADDRESS 0x7C

void setup() {
  Wire.begin(21, 22); // Initialize I2C communication with SDA on GPIO 21 and SCL on GPIO 22
  Serial.begin(9600);
  // Wire.onReceive(receiveEvent); // register receive event
  // Initialize the TM1650 display
  // Wire.beginTransmission(TM1650_ADDRESS);
  // Wire.write(0x8F); // Turn on the display
  // Wire.endTransmission();
}

void loop() {
  // Read the data meant for TM1650 display
  Wire.requestFrom(TM1650_ADDRESS, 1);
  // if (Wire.available()) {
  //   byte receivedData = Wire.read();
  //   // Print the received data to the console
  //   Serial.println(receivedData, HEX);

  int count = 0;
  byte address = 0;
  byte data = 0;

  while (Wire.available()) { 
    byte received = Wire.read(); 
    Serial.println(received, HEX);

    if (count == 0) {
      address = received;
      Serial.print("Address received: ");
      Serial.println(address, HEX);
    }
    else if (count == 1) {
      data = received;
      Serial.print("Data received: ");
      Serial.println(data, HEX);
    }
    count++;
  }

  // If the received address is the LED_ADDRESS, print the received data
  if (address == LED_ADDRESS) {
    Serial.print("LED data: ");
    Serial.println(data, BIN);
  }

  // If the received address is the DATA_ADDRESS, print the received data
  if (address == DATA_ADDRESS) {
    Serial.print("LED address: ");
    Serial.println(data, HEX);
  }

  delay(100); // Adjust the delay based on your requirements
}
