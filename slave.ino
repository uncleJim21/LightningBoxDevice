#include <Wire.h>

const int ADDR = 0x34;
int INDEX = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  INDEX = INDEX % 4;
  Wire.begin(INDEX + ADDR);  // Initialize I2C communication as a slave with address 0x50
  Wire.onReceive(receiveEvent);
  INDEX++;
  delay(100);
  Wire.end();
}


void receiveEvent(int numBytes) {
  while (Wire.available()) {
    byte receivedData = Wire.read();
    // Print the received data to the console
    Serial.print("Received on ");
    Serial.print(INDEX + ADDR);
    Serial.print(": 0x");
    Serial.println(receivedData, HEX);
  }
}

