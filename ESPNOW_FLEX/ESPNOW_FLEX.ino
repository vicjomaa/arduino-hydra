#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Receiver MAC address for ESP-NOW communication
uint8_t broadcastAddress[] = { 0x34, 0xB4, 0x72, 0xEA, 0x22, 0x94 };

// Structure to hold sensor data
typedef struct sensor_data {
  char data[128];  // Data from the sensor
  int id;          // Sensor ID
} sensor_data;

// Variable to hold sensor data to be sent
sensor_data mySensorData;

// Variable to hold peer information for ESP-NOW communication
esp_now_peer_info_t peerInfo;

// Analog pin A15 (mapped to digital pin 16)
int analogPin = A15;

// Interval for reading sensors (in milliseconds)
const unsigned long interval = 100;
unsigned long previousTime = 0;

// Interval for heartbeat signal (in milliseconds)
const unsigned long heartbeatInterval = 2000;
unsigned long previousTimeHB = 0;

// Interval for LED blinking (in milliseconds)
const unsigned long ledInterval = 1000;
unsigned long previousLedTime = 0;
bool ledState = LOW;

// Threshold for detecting significant changes in analog value
const float analogThreshold = 5.0;
int prevAnalogValue = 0;

// Callback function when data is sent via ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery fail");
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  
  Serial.begin(115200);

  
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback function for send status
  esp_now_register_send_cb(OnDataSent);

  // Set up peer information for ESP-NOW
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer to ESP-NOW
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Initialize the sensor data structure with an ID
  mySensorData.id = 5;  // Set the appropriate sensor ID

  // Set the analog read resolution to 12 bits (0-4095)
  analogReadResolution(12);

  // Read the initial analog value and store it
  prevAnalogValue = analogRead(analogPin);

  // Send initial data
  sendDataFormatted();

  // Initialize digital pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  
  unsigned long currentTime = millis();

  // Check if it's time to toggle the LED state
  if (currentTime - previousLedTime >= ledInterval) {
    previousLedTime = currentTime;  
    ledState = !ledState;          
    digitalWrite(LED_BUILTIN, ledState); 
  }

  // Check if it's time to read the sensor
  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;  // Update previous time

    // Read analog value from the specified pin
    int analogValue = analogRead(analogPin);
    Serial.print("analogData: ");
    Serial.println(analogValue);

    // Check for significant changes in analog value or heartbeat interval
    if (abs(analogValue - prevAnalogValue) > analogThreshold || currentTime - previousTimeHB >= heartbeatInterval) {
      
      previousTimeHB = currentTime;

    
      prevAnalogValue = analogValue;

     
      sendDataFormatted();
    }
  }
}

// Function to format and send sensor data via ESP-NOW
void sendDataFormatted() {
  snprintf(mySensorData.data, sizeof(mySensorData.data), "flex: %d", prevAnalogValue);
  esp_now_send(broadcastAddress, (uint8_t *)&mySensorData, sizeof(sensor_data));
}
