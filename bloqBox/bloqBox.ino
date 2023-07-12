#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>


const int ADDR = 0x34;
int INDEX = 0;
char digits[] = {'0','0','0','0'};


const char* ssid = "The Misfits";
const char* password = "";

String generateUUID() {
  // Initialize random number generator
  randomSeed(analogRead(0));
  char uuidString[37]; // UUID string buffer (36 characters + null terminator)

  // Generate random bytes for UUID
  uint8_t uuid[16];
  for (int i = 0; i < 16; i++) {
    uuid[i] = random(256);
  }

  // Format the UUID string
  sprintf(uuidString, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
          uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
          uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);

  return String(uuidString);
}

enum EventType {
  cancelPressed,
  confirmPressed,
  doorOpened,
  doorClosed,
  NONE
};

struct EventData {
  EventType eventType;
  String UUID;
  int cuckBucks;

  EventData(EventType type) {
    eventType = type;
    UUID = generateUUID();
    cuckBucks = -1;
  }
};

void makePostRequest(EventData eventData);

//Your Domain name with URL path or IP address with path
//String serverName = "https://api.ipify.org";
String updateBoxParamsEndpoint = "https://lightning-box.vercel.app/api/session/set";
String getSessionEndpoint = "https://lightning-box.vercel.app/api/session/get";

bool hasValidSession = false;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;
EventData eventArray[4] = {EventData(NONE),EventData(NONE),EventData(NONE),EventData(NONE)};
int currentEventIndex = 0;


WiFiClient wifiClient;
//HttpClient httpClient(wifiClient, serverAddress, serverPort);

void setup() {
  Serial.begin(115200);
  configWifi();

  eventArray[0] = EventData(cancelPressed);
  eventArray[1] = EventData(confirmPressed);
  eventArray[2] = EventData(doorOpened);
  eventArray[3] = EventData(doorClosed);

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
  INDEX = INDEX % 4;
  Wire.begin(INDEX + ADDR);  // Initialize I2C communication as a slave with address 0x50
  Wire.onReceive(receiveEvent);
  INDEX++;
  delay(100);
  Wire.end();
  // Serial.println("Current Total:");
  // Serial.println();
  // Serial.print(digits[3]);
  // Serial.print(digits[1]);
  // Serial.print(digits[2]);
  // Serial.print(digits[0]);
  EventData event = EventData(confirmPressed);
  EventData event2 = EventData(cancelPressed);
  // eventArray[0] = event;
  // eventArray[1] = event2;
  // currentEventIndex = 1;


  if ((millis() - lastTime) > timerDelay) {
    //(hasValidSession == false) ? (getSession()) : (makeHeartbeatRequest());
    getSession();
    
    event.cuckBucks = convertCuckBuckValue();
    makePostRequest(eventArray[3]);
    Serial.println("Current Total:");
    Serial.println();
    Serial.print(digits[3]);
    Serial.print(digits[1]);
    Serial.print(digits[2]);
    Serial.print(digits[0]);
  }

  //delay(5000);
  
}

void getSession(){
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = getSessionEndpoint;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());

      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("getSession HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        hasValidSession = (httpResponseCode == 200);
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

void makePostRequest(EventData eventData){
  //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = updateBoxParamsEndpoint;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Data to send with HTTP POST
      String httpRequestData;
      const String uuid = eventData.UUID;
      switch(eventData.eventType){
        case cancelPressed:
          httpRequestData = "event=cancelPressed&messageUUID=" + String(uuid);
        break;
        case confirmPressed:
          httpRequestData = "cuckBucks=" + String(eventData.cuckBucks) + "&event=confirmPressed&messageUUID=" + String(uuid);
        break;
        case doorOpened:
          httpRequestData = "event=doorOpened&messageUUID=" + String(uuid);
        break;
        case doorClosed:
          httpRequestData = "event=doorClosed&messageUUID=" + String(uuid);
        break;
      }

      

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      
      if (httpResponseCode>0) {
        Serial.print("makeHeartbeatRequest HTTP Response code: ");
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

int convertCuckBuckValue() {
  int value = 0;
  int multipliers[] = {1, 100, 10, 1000};

  for (int i = 0; i < 4; i++) {
    if(digits[i] == '-'){
      value += 0;
    }
    else{
      value += (digits[i] - '0') * multipliers[i];
    }
  }

  return value;
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

