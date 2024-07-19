#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_light.h>

// Wemos pins for I2C communication
#define SDA_PIN 3
#define SCL_PIN 5

// Create MPU6050 object
MPU6050 mpu(Wire);



// Broadcast address for ESP-NOW communication
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

// Define thresholds for sensor changes
const float accelThreshold = 0.1;  // Adjust as needed
const float gyroThreshold = 1.0;   // Adjust as needed

// Variables to store previous sensor values
float prevAx = 0, prevAy = 0, prevAz = 0;
float prevGx = 0, prevGy = 0, prevGz = 0;


// Callback function when data is sent via ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery fail");
  }
}


// Function to format and send sensor data via ESP-NOW
void sendDataFormatted() {
  snprintf(mySensorData.data, sizeof(mySensorData.data),
           "ax: %.2f, ay: %.2f, az: %.2f, gx: %.2f, gy: %.2f, gz: %.2f",
           prevAx, prevAy, prevAz, prevGx, prevGy, prevGz);
  esp_now_send(broadcastAddress, (uint8_t *)&mySensorData, sizeof(sensor_data));
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  // Initialize I2C communication
  Wire.begin(SDA_PIN, SCL_PIN);  // Specify the SDA and SCL pins

  // Initialize MPU6050
  if (mpu.begin() != 0) {
    Serial.println("MPU6050 initialization failed!");
    while (1)
      ;  // Halt if MPU6050 initialization failed
  }

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


  // Initialize sensor data structure with an ID
  mySensorData.id = 1;  // Set the appropriate sensor ID

  // Read initial MPU6050 data
  mpu.update();
  prevAx = mpu.getAccX();
  prevAy = mpu.getAccY();
  prevAz = mpu.getAccZ();
  prevGx = mpu.getGyroX();
  prevGy = mpu.getGyroY();
  prevGz = mpu.getGyroZ();

  // Send initial data
  sendDataFormatted();

  // Initialize digital pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);  

}

void loop() {
  // Get the current time in milliseconds
  unsigned long currentTime = millis();

  // Check if it's time to toggle the LED state
  if (currentTime - previousLedTime >= ledInterval) {
    previousLedTime = currentTime;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }

  // Check if it's time to read the sensor
  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;  // Update the previous time

    // Read MPU6050 data
    mpu.update();
    float ax = mpu.getAccX();
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();
    float gx = mpu.getGyroX();
    float gy = mpu.getGyroY();
    float gz = mpu.getGyroZ();

    // Check for significant changes in sensor readings or heartbeat interval
    if (abs(ax - prevAx) > accelThreshold || abs(ay - prevAy) > accelThreshold || abs(az - prevAz) > accelThreshold || abs(gx - prevGx) > gyroThreshold || abs(gy - prevGy) > gyroThreshold || abs(gz - prevGz) > gyroThreshold || currentTime - previousTimeHB >= heartbeatInterval) {
      previousTimeHB = currentTime;  // Update heartbeat time

      // Update previous values
      prevAx = ax;
      prevAy = ay;
      prevAz = az;
      prevGx = gx;
      prevGy = gy;
      prevGz = gz;

      // Send the formatted data
      sendDataFormatted();
    }
  }
}


