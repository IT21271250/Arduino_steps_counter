#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>


const char WIFI_SSID[] = "Dineth";
const char WIFI_PASSWORD[] = "11111111";
const char MQTT_BROKER_ADDRESS[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = "Fitness tracker-esp32-001";
const char PUBLISH_TOPIC_STEPS[] = "Fitness tracker-esp32-001/steps";
const char PUBLISH_TOPIC_FALL[] = "Fitness tracker-esp32-001/fall";
const char SUBSCRIBE_TOPIC[] = "Fitness tracker-esp32-001/loopback";
const char RESET_COMMAND[] = "reset";  // Command for resetting the step count

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

WiFiClient network;
MQTTClient mqtt(512);

int stepCounter = 0;
bool lastStepDetected = false;
float stepThresholdAccel = 2.0;
float stepThresholdGyro = 1.0;
float fallThresholdHigh = 3.0;
float fallThresholdLow = -30.0;
bool fallDetected = false;
bool resetFlag = false;  // Flag to indicate reset status

unsigned long lastPublishTime = 0;
const int PUBLISH_INTERVAL = 5000;

void setup() {
    Serial.begin(115200);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true);
    }
    display.display();
    delay(2000);
    display.clearDisplay();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to Wi-Fi...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to Wi-Fi");

    connectToMQTT();

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (true) {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");

mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
    }

    if (!mqtt.connected()) {
        Serial.println("MQTT disconnected. Reconnecting...");
        connectToMQTT();
    }
    mqtt.loop();

    if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
        sendToMQTT();
        lastPublishTime = millis();
    }

    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;
    float gz = gyro.gyro.z;

    if ((az > stepThresholdAccel || az < -stepThresholdAccel) && abs(gz) > stepThresholdGyro) {
        if (!lastStepDetected) {
            stepCounter++;
            Serial.print("Step detected! Total steps: ");
            Serial.println(stepCounter);
            lastStepDetected = true;
        }
    } else {
        lastStepDetected = false;
    }

    // Display on OLED
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Steps: ");
    display.println(stepCounter);
    display.display();

    // Fall Detection

// Replace with your network credentials
const char* ssid = "Champika";        // Your Wi-Fi SSID
const char* password = "11111111";  // Your Wi-Fi Password

// OLED display configuration
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

// Web server on port 80
WebServer server(80);

int stepCounter = 0;
float previousZ = 0;
float stepThreshold = 2.0;  // Step detection threshold
float fallThresholdHigh = 3.0;  // Fall detection upper threshold
float fallThresholdLow = -30.0; // Fall detection lower threshold
bool fallDetected;

// Function to handle root URL
void handleRoot() {
    String html = "<!DOCTYPE html><html lang='en'>";
  html += "<head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Health Device Dashboard</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 600px; margin: 0 auto; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1); }";
  html += ".status { font-size: 1.5em; }";
  html += ".step-count { font-weight: bold; color: #2c3e50; }";
  html += ".fall-status { font-weight: bold; color: " + String(fallDetected ? "#e74c3c" : "#27ae60") + "; }"; // Use different colors for fall detection status
  html += ".footer { margin-top: 20px; font-size: 0.9em; color: #888; text-align: center; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1> Health Device Dashboard</h1>";
  html += "<p class='status'>Steps: <span class='step-count'>" + String(stepCounter-1) + "</span></p>";
  html += "<p class='status'>Fall Detected: <span class='fall-status'>" + String(fallDetected ? "Yes" : "No") + "</span></p>";
  html += "<div class='footer'>Powered by ESP32</div>";
  html += "</div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);  // Stop execution if OLED initialization fails
  }
  display.display();  // Show splash screen
  delay(2000);        // Wait 2 seconds
  display.clearDisplay();  // Clear display buffer

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();  // Show connecting message on OLED

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Once connected, print the IP address
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());  // Print IP address to Serial Monitor

  // Display the IP address on OLED display
  display.clearDisplay();  // Clear the display buffer
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.print("IP Address: ");
  display.println(WiFi.localIP());  // Show IP address on OLED
  display.display();  // Update the display

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (true) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  // Set accelerometer and gyroscope ranges
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  // Define the root URL handler
 server.on("/", handleRoot);

 // Start the server
  server.begin();  
  Serial.println("HTTP server started");
}

void loop() {

  server.handleClient();
  
  // Get sensor readings
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  // Step Detection
  if ((az - previousZ) > stepThreshold) {
   
    Serial.print("Step detected! Total steps: ");
    Serial.println(stepCounter);
    stepCounter++;

    // Display step count on OLED
    display.clearDisplay();
    display.setTextSize(2);  
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Steps: ");
    display.println(stepCounter-1);
    display.display();
    delay(500);
  }

   // Fall Detection

  if (ax > fallThresholdHigh || ay > fallThresholdHigh || az > fallThresholdHigh || 
      ax < fallThresholdLow || ay < fallThresholdLow || az < fallThresholdLow) {

    // If no fall, keep monitoring status
    display.setTextSize(1);
    display.setCursor(0, 16);
    fallDetected = false;
    display.println("Monitoring...");
    display.display();
  
  } else {
    
    // Display fall detected on OLED

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    fallDetected = true;
    display.println("Fall");
    display.println("Detected!");
    display.display();
    Serial.println("Fall detected!"); 
  }

  // Update Z value for step detection
  previousZ = az;  

  // Delay for half a second
  delay(300); 
}
