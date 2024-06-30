#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Receiver MAC address
uint8_t broadcastAddress[] = { 0x34, 0xB4, 0x72, 0xEA, 0x22, 0x94 };

typedef struct sensor_data {
  char data[128];  // Data from the sensor
  int id;          // Sensor ID
} sensor_data;

// Send message via ESP-NOW
sensor_data mySensorData;

esp_now_peer_info_t peerInfo;

// Analog pin
int analogPin = 34;

// Interval for reading sensors (in milliseconds)
const unsigned long interval = 100;
unsigned long previousTime = 0;

// Interval for heartbeat
const unsigned long heartbeatInterval = 2000;
unsigned long previousTimeHB = 0;

// Variables to store previous sensor value
const float analogThreshold = 5.0;
int prevAnalogValue = 0;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery fail");
  }
}

void setup() {
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback function for send status
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Initialize the sensor data structure with an ID
  mySensorData.id = 2;  // Set the appropriate sensor ID

  //set the resolution to 12 bits (0-4095)
  analogReadResolution(12);

  // Send initial data
  prevAnalogValue = analogRead(analogPin);
  sendDataFormatted();
}

void loop() {
  unsigned long currentTime = millis();  // Get current time

  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;  // Update previous time

    // Read analog value from A0
    int analogValue = analogRead(analogPin);


    // Check for significant changes in analog value
    if (abs(analogValue - prevAnalogValue) > analogThreshold || currentTime - previousTimeHB >= heartbeatInterval) {

      // Update time
      previousTimeHB = currentTime;

      // Update previous value
      prevAnalogValue = analogValue;

      sendDataFormatted();
    }
  }
}

void sendDataFormatted() {
  snprintf(mySensorData.data, sizeof(mySensorData.data),
           "flex: %d", prevAnalogValue);
  esp_now_send(broadcastAddress, (uint8_t *)&mySensorData, sizeof(sensor_data));
}
