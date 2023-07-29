#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include <ArduinoJson.h>


const int ADDR = 0x34;
int INDEX = 0;
char digits[] = {'0','0','0','0'};

//Inputs/Output definitions
const int lockPin = 23;
const int doorSensorPin = 25;//
const int billCounterCtlPin = 18;//
const int ledPin = 27;
const int cancelButtonPin = 16;//
const int confirmButtonPin = 17;//

bool remoteLNURLisSet = false;
bool remoteInvoiceIsPaid = false;
bool remoteBalanceOK = false;
bool LED_blink_flipflop = false;

enum SystemState {
  awaitingLNURL,
  awaitingDoorClose,
  provisionalDeposit,
  locked
};

void setPinsForState(SystemState state);

//Your Domain name with URL path or IP address with path
//String serverName = "https://api.ipify.org";
String updateBoxParamsEndpoint = "https://lightning-box-cmdruid.vercel.app/api/box";
String getSessionEndpoint = "https://lightning-box.vercel.app/api/session/get";

bool hasValidSession = false;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long longLoopPeriod = 600000;
// Set timer to 5 seconds (5000)
unsigned long longLoopPeriod = 2000;
unsigned long shortLoopPeriod = 25;
int currentEventIndex = 0;
SystemState currentState = awaitingLNURL;


WiFiClient wifiClient;
//HttpClient httpClient(wifiClient, serverAddress, serverPort);

void setup() {
  Serial.begin(115200);
  configWifi();
  configurePins();

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


  if ((millis() - lastTime) > longLoopPeriod) {
    //(hasValidSession == false) ? (getSession()) : (makeHeartbeatRequest());
    makePostRequest();
    Serial.println("Current Total:");
    Serial.println();
    Serial.print(digits[3]);
    Serial.print(digits[1]);
    Serial.print(digits[2]);
    Serial.print(digits[0]);
  }

  if ((millis() - lastTime) > shortLoopPeriod) {
    updateState();
  }



  //delay(5000);
  
}

String getStateString() {
  switch (currentState) {
    case awaitingLNURL:
      return "await_addr";
    case awaitingDoorClose:
      return "await_door";
    case provisionalDeposit:
      return "depositing";
    case locked:
      return "locked";
    default:
      return "unknown";
  }
}


#include <ArduinoJson.h>

void makePostRequest(){
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String serverPath = updateBoxParamsEndpoint;
    
    // Your Domain name with URL path or IP address with path
    http.begin(serverPath.c_str());
    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
    http.addHeader("token", api_token);

    // Create a JSON document to hold the response data
    DynamicJsonDocument jsonDoc(1024); // Adjust the buffer size as per your response data size

    // Data to send with HTTP POST
    const int boxValue = convertCuckBuckValue();
    const String boxState = getStateString();

    // Populate the JSON document with the required fields
    jsonDoc["code"] = "12345";
    jsonDoc["amount"] = boxValue;
    jsonDoc["state"] = boxState;
    //jsonDoc["addr_set"] = remoteLNURLisSet;

    // Serialize the JSON document into a String
    String payload;
    serializeJson(jsonDoc, payload);

    // Send HTTP POST request
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode > 0) {
      Serial.print("makeHeartbeatRequest HTTP Response code: ");
      Serial.println(httpResponseCode);
      
      // Parse the JSON response received from the server
      DeserializationError error = deserializeJson(jsonDoc, http.getString());
      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
      } else {
        // Extract the boolean values of amount_ok, addr_ok, and is_paid
        remoteBalanceOK = jsonDoc["amount_ok"];
        remoteLNURLisSet = jsonDoc["addr_ok"];
        remoteInvoiceIsPaid = jsonDoc["is_paid"];

        // Print the extracted boolean values to the Serial Monitor
        Serial.print("amount_ok: ");
        Serial.println(remoteBalanceOK);
        Serial.print("addr_ok: ");
        Serial.println(remoteLNURLisSet);
        Serial.print("is_paid: ");
        Serial.println(remoteInvoiceIsPaid);
      }
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    Serial.print("confirmButton: ");
    Serial.println(digitalRead(confirmButtonPin));
    Serial.print("cancelButton: ");
    Serial.println(digitalRead(cancelButtonPin));
    Serial.print("doorSensorPin: ");
    Serial.println(digitalRead(doorSensorPin));
    Serial.print("currentState: ");
    Serial.println(getStateString());
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  lastTime = millis();
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


void updateState(){
  const SystemState lastState = currentState;
  bool shouldPopLock = false;
  switch(currentState){
    case awaitingLNURL:
      if (remoteLNURLisSet) {
        currentState = awaitingDoorClose;
      }
      break;
    case awaitingDoorClose:
      //
      if (digitalRead(cancelButtonPin) == LOW){//kick out on cancellation
        currentState = awaitingLNURL;
        shouldPopLock = true;
      }
      else if (digitalRead(doorSensorPin) == LOW){//waiting for door closure
        currentState = provisionalDeposit;
      }
      break;
    case provisionalDeposit:
      if (digitalRead(cancelButtonPin) == LOW){//waiting for
        currentState = awaitingLNURL;
        shouldPopLock = true;
      }
      else if (digitalRead(confirmButtonPin) == LOW && remoteBalanceOK == true){
        currentState = locked;
      }
      break;
    case locked:
      if(remoteInvoiceIsPaid == true){
        currentState = awaitingLNURL;
        remoteLNURLisSet = false;
        shouldPopLock = true;
      }
    break;
  }

  if(currentState != lastState){
    setPinsForState(currentState);
    if (shouldPopLock == true){//pop the lock once and only once
      digitalWrite(lockPin,HIGH);
      delay(2000);
      digitalWrite(lockPin,LOW);
      for (int i = 0; i < 4; i++) {
        digits[i] = 0;
      }
    }
  }
}

void setPinsForState(SystemState state){
  switch(currentState){
    case awaitingLNURL:
      digitalWrite(lockPin,LOW);
      digitalWrite(billCounterCtlPin,LOW);
      digitalWrite(ledPin,LOW);
    break;
    case awaitingDoorClose:
      digitalWrite(lockPin,LOW);
      digitalWrite(billCounterCtlPin,LOW);
      digitalWrite(ledPin,LOW);
    break;
    case provisionalDeposit:
      digitalWrite(billCounterCtlPin,HIGH);
      LED_blink_flipflop = !LED_blink_flipflop;
      digitalWrite(ledPin,LED_blink_flipflop);
      
    break;
    case locked:
    digitalWrite(billCounterCtlPin,LOW);
    digitalWrite(ledPin,HIGH);
    break;
  }
}

void configurePins(){
  pinMode(ledPin, OUTPUT);
  pinMode(billCounterCtlPin, OUTPUT);
  pinMode(lockPin, OUTPUT);

  pinMode(cancelButtonPin, INPUT);
  pinMode(confirmButtonPin, INPUT);
  pinMode(doorSensorPin, INPUT);
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

