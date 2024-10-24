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

 if (ax > fallThresholdHigh || ay > fallThresholdHigh || az > fallThresholdHigh || 
        ax < fallThresholdLow || ay < fallThresholdLow || az < fallThresholdLow) {

          fallDetected = false;
        display.setTextSize(1);
        display.setCursor(0, 16);
        display.println("Monitoring...");
        display.display();
       
    } else {
         fallDetected = true;
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println("Fall");
        display.println("Detected!");
        display.display();
        Serial.println("Fall detected!");
    }

    // Check if the reset flag is set
    if (resetFlag) {
        resetStepCounter();
    }

    delay(300);  // Delay for half a second
}

// Function to connect to MQTT broker
void connectToMQTT() {
    mqtt.begin(MQTT_BROKER_ADDRESS, MQTT_PORT, network);
    mqtt.onMessage(messageHandler);

    Serial.print("Connecting to MQTT broker...");
    while (!mqtt.connect(MQTT_CLIENT_ID)) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("Connected to MQTT broker!");

    if (mqtt.subscribe(SUBSCRIBE_TOPIC)) {
        Serial.print("Subscribed to: ");
        Serial.println(SUBSCRIBE_TOPIC);
    } else {
        Serial.print("Failed to subscribe to: ");
        Serial.println(SUBSCRIBE_TOPIC);
    }
}

        delay(1000);

// Function to handle incoming MQTT messages
void messageHandler(String &topic, String &payload) {
    Serial.println("Received from MQTT:");
    Serial.println("- topic: " + topic);
    Serial.println("- payload: " + payload);

    // Check for the reset command
    if (payload == RESET_COMMAND) {
        resetFlag = true;  // Set flag to reset the step counter
        Serial.println("Reset command received, resetting step count...");
    }
}

// Function to reset step counter
void resetStepCounter() {
    stepCounter = 0;  // Reset step count
    resetFlag = false;  // Clear the reset flag
    Serial.println("Step counter has been reset.");
}
