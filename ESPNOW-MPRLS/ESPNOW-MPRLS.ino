#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPRLS.h>

// Libraries for ESP-NOW
#include <espnow.h>
#include <ESP8266WiFi.h>

// Define I2C addresses for the sensors
#define MPRLS_ADDRESS 0x18   // Default I2C address for Adafruit MPRLS

// Receiver MAC address
uint8_t broadcastAddress[] = {0x34, 0xB4, 0x72, 0xEA, 0x22, 0x94};

// Structure example to send data
// Must match the receiver structure
typedef struct mprls_data {
  float pressure_hPa;
} mprls_data_data;

// Create a struct_message called myData
mprls_data data;


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  /*Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }*/
  
}
// Create sensor objects
Adafruit_MPRLS mprls = Adafruit_MPRLS(MPRLS_ADDRESS);

// Interval for reading sensors (in milliseconds)
const unsigned long interval = 100;
unsigned long previousTime = 0;

// Interval for proof of live
const unsigned long intervalHB = 2000;
unsigned long previousTimeHB = 0;

// Define thresholds for sensor changes
const float pressureThreshold = 1.0; // Adjust as needed
float prevPressure = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin();

  if (!mprls.begin()) {
    while (1); // MPRLS initialization failed
  }

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;


  //Init value

  prevPressure = mprls.readPressure();
  data.pressure_hPa = prevPressure;
  esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));


  }

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
}

void loop() {
  unsigned long currentTime = millis(); // Get current time


  if(currentTime - previousTimeHB >= intervalHB){
    previousTimeHB = currentTime; // Update previous time

    // Prepare a simple heartbeat message
    const char* heartbeatMessage = "sensor2";
    
    // Send the heartbeat message
    esp_now_send(broadcastAddress, (uint8_t *)heartbeatMessage, strlen(heartbeatMessage) + 1);


  }
  if (currentTime - previousTime >= interval) {
    previousTime = currentTime; // Update previous time

    // Read Adafruit MPRLS data
    float pressure_hPa = mprls.readPressure();

    // Check for significant changes in sensor readings
    if (abs(pressure_hPa - prevPressure) > pressureThreshold) {

      prevPressure = pressure_hPa;

      // Set values to send
      data.pressure_hPa = pressure_hPa;

      // Send message via ESP-NOW
      esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));

     
    }
  }
}
