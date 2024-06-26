#include <esp_now.h>
#include <WiFi.h>

// Structure for pressure sensor data
typedef struct {
  float pressure_hPa;
} PressureData;

// Structure for accelerometer sensor data
typedef struct {
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
} AccelerometerData;

// Instances of sensor data
PressureData pressure;
AccelerometerData accelerometer;

// MAC addresses of the sensors
uint8_t accelerometerMac[] =  {0xA0, 0x20, 0xA6, 0x13, 0x11, 0x60}; // Example MAC address
uint8_t pressureSensorMac[] = {0xE0, 0x98, 0x06, 0x0D, 0xC8, 0xF1}; // Example MAC address

// Flags to track sensor activity and first data reception
bool isAccelerometerActive = false;
bool isPressureSensorActive = false;
bool firstDataFromAccel = false;
bool firstDataFromPressure = false;

// Timestamps for the last heartbeat message received from each sensor
unsigned long lastAccelHeartbeat = 0;
unsigned long lastPressureHeartbeat = 0;

unsigned long lastDataRefresh = 0;

// Interval for proof of life
const unsigned long heartbeatInterval = 2000;

unsigned long currentTime;

void onDataReceived(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  // Extract the MAC address of the sender from the info structure
  uint8_t *mac = info->src_addr;

  if (memcmp(mac, accelerometerMac, 6) == 0) {
    lastAccelHeartbeat = currentTime;
  }

  if (memcmp(mac, pressureSensorMac, 6) == 0) {
    lastPressureHeartbeat = currentTime;
  }

  if (len == sizeof(PressureData)) {
    memcpy(&pressure, incomingData, sizeof(PressureData));
    sendMessage();
    firstDataFromPressure = true;
  }  
  
  if (len == sizeof(AccelerometerData)) {
    memcpy(&accelerometer, incomingData, sizeof(AccelerometerData));
    sendMessage();
    firstDataFromAccel = true;
  }
}

void sendMessage() {
  // Print data in the desired JSON-like format
  Serial.print("{");
  
  if (isAccelerometerActive && firstDataFromAccel) {
    Serial.print("ax:");
    Serial.print(accelerometer.ax);
    Serial.print(", ay:");
    Serial.print(accelerometer.ay);
    Serial.print(", az:");
    Serial.print(accelerometer.az);
    Serial.print(", gx:");
    Serial.print(accelerometer.gx);
    Serial.print(", gy:");
    Serial.print(accelerometer.gy);
    Serial.print(", gz:");
    Serial.print(accelerometer.gz);
  }
  
  if (isAccelerometerActive && isPressureSensorActive) {
    Serial.print(",");
  }
  
  if (isPressureSensorActive && firstDataFromPressure) {
    Serial.print("pressure_hPa:");
    Serial.print(pressure.pressure_hPa);
  }
  
  Serial.println("}");
}

void setup() {
  // Initialize Serial Monitor
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

  if (currentTime - lastDataRefresh >= heartbeatInterval) {
    lastDataRefresh = currentTime;
    sendMessage(); // Update previous data
  }

  isAccelerometerActive = currentTime - lastAccelHeartbeat < heartbeatInterval;
  isPressureSensorActive = currentTime - lastPressureHeartbeat < heartbeatInterval;
}
