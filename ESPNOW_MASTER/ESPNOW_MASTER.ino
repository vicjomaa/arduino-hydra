#include <esp_now.h>
#include <WiFi.h>

// Structure for any sensor
typedef struct sensor_data {
  char data[128]; // Data from the sensor
  int id;
} sensor_data;

sensor_data mySensorData;

// MAC addresses of the receivers -> TO ADD MORE SENSORS JUST INSERT A NEW MAC ADDRESS
uint8_t receiverAddress[][6] = {
  {0xA0, 0x20, 0xA6, 0x13, 0x11, 0x60},
  {0xE0, 0x98, 0x06, 0x0D, 0xC8, 0xF1},
  {0x8C, 0x4B, 0x14, 0x0B, 0x12, 0x0C},
  {0x8C, 0x4B, 0x14, 0x0F, 0x3F, 0xD4}
};

// Calculate the number of sensors
const int MAX_SENSORS = sizeof(receiverAddress) / sizeof(receiverAddress[0]);

unsigned long lastDataReceived[MAX_SENSORS] = {0};
bool isSensorAlive[MAX_SENSORS] = {false};
String messageToHydra[MAX_SENSORS] = {""};

// windowSendData for proof of life 
const unsigned long timeWindowSensors = 3000;

unsigned long currentTime;
const unsigned long windowSendData = 100;
unsigned long previousTime = 0;

// Callback function to handle received data
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(sensor_data)) {
    memcpy(&mySensorData, incomingData, sizeof(sensor_data));
    lastDataReceived[mySensorData.id] = millis();
    messageToHydra[mySensorData.id] = mySensorData.data;
    sendMessage();
  }
}

void sendMessage() {
  // Print data in the desired JSON-like format
  Serial.print("{");
  int validData = 0;
  for (int i = 0; i < MAX_SENSORS; i++) {
    if (isSensorAlive[i] && !messageToHydra[i].isEmpty()) {
      if(validData != 0){
         Serial.print(",");
      }
      Serial.print(messageToHydra[i]);
      validData ++;
    }
  }
  Serial.println("}");
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
  
  // Register callback function for received data
  esp_now_register_recv_cb(onDataReceived);


}

void loop() {
  currentTime = millis(); // Get current time

  if (currentTime - previousTime >= windowSendData) {
    previousTime = currentTime;
    sendMessage(); // Update previous data
  }

  for (int i = 0; i < MAX_SENSORS; i++) {
    isSensorAlive[i] = checkSensorAlive(lastDataReceived[i]);
  }
}

bool checkSensorAlive(unsigned long lastTime) {
  return currentTime - lastTime <= timeWindowSensors;
}
