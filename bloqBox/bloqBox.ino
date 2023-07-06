#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>


const int ADDR = 0x34;
int INDEX = 0;
char digits[] = {'0','0','0','0'};

const char* ssid = "The Misfits";
const char* password = "";

//Your Domain name with URL path or IP address with path
String serverName = "https://api.ipify.org";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

WiFiClient wifiClient;
//HttpClient httpClient(wifiClient, serverAddress, serverPort);

void setup() {
  Serial.begin(115200);
  configWifi();
}


void configWifi(){
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // INDEX = INDEX % 4;
  // Wire.begin(INDEX + ADDR);  // Initialize I2C communication as a slave with address 0x50
  // Wire.onReceive(receiveEvent);
  // INDEX++;
  // delay(100);
  // Wire.end();
  // Serial.println("Current Total:");
  // Serial.println();
  // Serial.print(digits[3]);
  // Serial.print(digits[1]);
  // Serial.print(digits[2]);
  // Serial.print(digits[0]);
  
  if ((millis() - lastTime) > timerDelay) {
    makeHeartbeatRequest();
  }

  delay(5000);
  
}

void makeHeartbeatRequest(){
  //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = serverName;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
}

char lookupTable(uint8_t input) {
  // Define the lookup table
  const uint8_t inputValues[] = {0xD7, 0x11, 0xCD, 0x5D, 0x1B, 0x5E, 0xDE, 0x15, 0xDF, 0x5F};
  const char outputValues[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  const uint8_t tableSize = sizeof(inputValues) / sizeof(inputValues[0]);
  // Find the index of the input value in the lookup table
  for (uint8_t i = 0; i < tableSize; i++) {
    if (input == inputValues[i]) {
      return outputValues[i];
    }
  }
  // Return a default value if the input value is not found
  return '-';
}

void receiveEvent(int numBytes) {
  while (Wire.available()) {
    byte receivedData = Wire.read();
    // Print the received data to the console
    const int currentAddr = INDEX + ADDR;

    char decodedData = lookupTable(receivedData);
    bool debug = false;//flip to enable/disable verbose prints
    if(debug){
      Serial.print("Received on ");
      Serial.print(currentAddr);
      Serial.print(": ");
      Serial.println(decodedData);
      Serial.println();
      
      Serial.println(INDEX <= 3);
      Serial.println(INDEX);
    }

    if(currentAddr <= 56 && currentAddr >= 53){
      digits[currentAddr - 53] = decodedData;
    }

    //digits[INDEX] = decodedData;
  }
}

