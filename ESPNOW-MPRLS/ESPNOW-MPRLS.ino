#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPRLS.h>
#include <espnow.h>
#include <ESP8266WiFi.h>

#define MPRLS_ADDRESS 0x18  // Default I2C address for Adafruit MPRLS

uint8_t broadcastAddress[] = { 0x34, 0xB4, 0x72, 0xEA, 0x22, 0x94 };

typedef struct sensor_data {
  char data[128];  // Data from the sensor
  int id;
} sensor_data;

sensor_data mySensorData;

Adafruit_MPRLS mprls = Adafruit_MPRLS(MPRLS_ADDRESS);

// Interval for reading sensors (in milliseconds)
const unsigned long interval = 100;
unsigned long previousTime = 0;

//Ping state of the sesnor
const unsigned long heartbeatInterval = 2000;
unsigned long previousTimeHB = 0;

// Define thresholds for sensor changes
const float pressureThreshold = 1.0;
float prevPressure = 0;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery fail");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

 

  if (!mprls.begin()) {
    while (1);  // MPRLS initialization failed
  }

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Init Data
  prevPressure = mprls.readPressure();

  // Initialize the sensor data structure with an ID
  mySensorData.id = 0; // Set the appropriate sensor ID

  sendDataFormated();

}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - previousTime >= interval) {
    
    float pressure_hPa = mprls.readPressure();
    previousTime = currentTime;  // Update previous time

    if (abs(pressure_hPa - prevPressure) > pressureThreshold || currentTime - previousTimeHB >= heartbeatInterval) {
      previousTimeHB = currentTime;
      prevPressure = pressure_hPa;
      sendDataFormated();
    }
  }
}

void sendDataFormated() {
  snprintf(mySensorData.data, sizeof(mySensorData.data), "pressure: %.2f", prevPressure);
  esp_now_send(broadcastAddress, (uint8_t *)&mySensorData, sizeof(sensor_data));
}
