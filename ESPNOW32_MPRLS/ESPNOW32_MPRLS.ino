#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPRLS.h>

// Wemos pins for I2C communication
#define SDA_PIN 3
#define SCL_PIN 5

#define MPRLS_ADDRESS 0x18  // Default I2C address for Adafruit MPRLS

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

// Create an instance of the Adafruit_MPRLS sensor
Adafruit_MPRLS mprls = Adafruit_MPRLS(MPRLS_ADDRESS);

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

// Threshold for detecting significant pressure changes
const float pressureThreshold = 1.0;
float prevPressure = 0;

// Callback function when data is sent via ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery fail");
  }
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  // Initialize I2C communication
  Wire.begin(SDA_PIN, SCL_PIN);  // Specify the SDA and SCL pins

  // Initialize the MPRLS sensor
  if (!mprls.begin()) {
    while (1)
      ;  // Halt if MPRLS initialization failed
  }

  // Set WiFi mode to station and disconnect from any network
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
  mySensorData.id = 0;  // Set the appropriate sensor ID

  // Initialize previous pressure reading
  prevPressure = mprls.readPressure();

  // Send initial data
  sendDataFormated();

  // Initialize digital pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);

  
  Serial.println("Scanning I2C devices...");
  byte count = 0;
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at address 0x");
      if (i < 16) Serial.print("0");
      Serial.println(i, HEX);
      count++;
      delay(1);  // Delay to prevent overloading the I2C bus
    }
  }
  Serial.print("Found ");
  Serial.print(count, DEC);
  Serial.println(" device(s).");
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
    // Read pressure from the sensor
    float pressure_hPa = mprls.readPressure();
    previousTime = currentTime;  // Update the previous time

    // Check if there is a significant change in pressure or it's time for a heartbeat signal
    if (abs(pressure_hPa - prevPressure) > pressureThreshold || currentTime - previousTimeHB >= heartbeatInterval) {
      previousTimeHB = currentTime;
      prevPressure = pressure_hPa;
      sendDataFormated();  // Send the formatted data
    }
  }
}

// Function to format and send sensor data via ESP-NOW
void sendDataFormated() {
  snprintf(mySensorData.data, sizeof(mySensorData.data), "pressure: %.2f", prevPressure);
  esp_now_send(broadcastAddress, (uint8_t *)&mySensorData, sizeof(sensor_data));
}
