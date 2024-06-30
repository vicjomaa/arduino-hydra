# 1 "D:\\ThesisProjekt\\Arduino\\ESPNOW\\arduino-hydra\\ESPNOW_MASTER\\ESPNOW_MASTER.ino"
# 2 "D:\\ThesisProjekt\\Arduino\\ESPNOW\\arduino-hydra\\ESPNOW_MASTER\\ESPNOW_MASTER.ino" 2
# 3 "D:\\ThesisProjekt\\Arduino\\ESPNOW\\arduino-hydra\\ESPNOW_MASTER\\ESPNOW_MASTER.ino" 2

// Structure for any sensor
typedef struct sensor_data {
  char data[64]; // Data from the sensor
} sensor_data;

sensor_data mySensorData;



// MAC addresses of the receivers -> TO ADD MORE SENSORS JUST INSERT A NEW MAC ADRESS
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

// Interval for proof of life
const unsigned long heartbeatInterval = 2000;

unsigned long currentTime;
unsigned long lastDataRefresh = 0; // Variable to track the last data refresh time

// Callback function to handle received data
void onDataReceived(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  int index = -1;
  // Extract the MAC address of the sender from the info structure
  uint8_t *mac = info->src_addr;

  // Check the MAC to update the live state
  for (int i = 0; i < MAX_SENSORS; i++) {
    if (memcmp(mac, receiverAddress[i], 6) == 0) {
      lastDataReceived[i] = millis();
      index = i;
      break;
    }
  }

  if (index != -1 && len == sizeof(sensor_data)) {
    memcpy(&mySensorData, incomingData, sizeof(sensor_data));
    messageToHydra[index] = mySensorData.data;
  }
}

void sendMessage() {
  // Print data in the desired JSON-like format
  HWCDCSerial.print("{");

  for (int i = 0; i < MAX_SENSORS; i++) {
    if (isSensorAlive[i] && messageToHydra[i] != "") {
      HWCDCSerial.print(messageToHydra[i]);
      HWCDCSerial.print(",");
    }
  }
  HWCDCSerial.println("}");
}



void setup() {

  HWCDCSerial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_MODE_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0 /*!< esp_err_t value indicating success (no error) */) {
    HWCDCSerial.println("Error initializing ESP-NOW");
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

  for (int i = 0; i < MAX_SENSORS; i++) {
    isSensorAlive[i] = checkSensorAlive(lastDataReceived[i]);
  }
}

bool checkSensorAlive(unsigned long lastTime) {
  return currentTime - lastTime < heartbeatInterval;
}
