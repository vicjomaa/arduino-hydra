#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>

// Libraries for ESP-NOW
#include <espnow.h>
#include <ESP8266WiFi.h>


// Define I2C addresses for the sensors
#define MPU6050_ADDRESS 0x68  // MPU-6050 I2C address


// Create sensor objects
MPU6050 mpu(Wire);

// Receiver MAC address
uint8_t broadcastAddress[] = { 0x34, 0xB4, 0x72, 0xEA, 0x22, 0x94 };

typedef struct sensor_data {
  char data[128];  // Data from the sensor
  int id;
} sensor_data;


// Send message via ESP-NOW
sensor_data mySensorData;


// Interval for reading sensors (in milliseconds)
const unsigned long interval = 100;
unsigned long previousTime = 0;


//Ping state of the sesnor
const unsigned long heartbeatInterval = 2000;
unsigned long previousTimeHB = 0;


// Define thresholds for sensor changes
const float accelThreshold = 0.1;  // Adjust as needed
const float gyroThreshold = 1.0;   // Adjust as needed

// Variables to store previous sensor values
float prevAx = 0, prevAy = 0, prevAz = 0;
float prevGx = 0, prevGy = 0, prevGz = 0;


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
  if (mpu.begin() != 0) {
    while (1)
      ;  // MPU6050 initialization failed
  }

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
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



  //Sent first data
  prevAx = mpu.getAccX();
  prevAy = mpu.getAccY();
  prevAz = mpu.getAccZ();
  prevGx = mpu.getGyroX();
  prevGy = mpu.getGyroY();
  prevGz = mpu.getGyroZ();


  // Initialize the sensor data structure with an ID
  mySensorData.id = 1; // Set the appropriate sensor ID


  sendDataFormated();
}

void loop() {


  unsigned long currentTime = millis();  // Get current time


  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;  // Update previous time

    // Read MPU-6050 data
    mpu.update();
    float ax = mpu.getAccX();
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();
    float gx = mpu.getGyroX();
    float gy = mpu.getGyroY();
    float gz = mpu.getGyroZ();


    // Check for significant changes in sensor readings
    if (abs(ax - prevAx) > accelThreshold || abs(ay - prevAy) > accelThreshold || abs(az - prevAz) > accelThreshold || abs(gx - prevGx) > gyroThreshold || abs(gy - prevGy) > gyroThreshold || abs(gz - prevGz) > gyroThreshold || currentTime - previousTimeHB >= heartbeatInterval) {

      //Update Time
      previousTimeHB = currentTime;
      // Update previous values
      prevAx = ax;
      prevAy = ay;
      prevAz = az;
      prevGx = gx;
      prevGy = gy;
      prevGz = gz;

      sendDataFormated();
    }
  }
}

void sendDataFormated() {
  snprintf(mySensorData.data, sizeof(mySensorData.data),
          "ax: %.2f, ay: %.2f, az: %.2f, gx: %.2f, gy: %.2f, gz: %.2f",
           prevAx, prevAy, prevAz, prevGx, prevGy, prevGz);
  esp_now_send(broadcastAddress, (uint8_t *)&mySensorData, sizeof(sensor_data));
}
