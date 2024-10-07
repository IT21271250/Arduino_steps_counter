//libraries
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// OLED display configuration

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MPU6050 mpu;

int stepCounter = 0;
float previousZ = 0;
float stepThreshold = 1.5;  // Step detection threshold
float fallThresholdHigh = 3.0;  // Fall detection upper threshold
float fallThresholdLow = -14.0; // Fall detection lower threshold

void setup(void) {
  Serial.begin(115200);  // Ensure this line is correct and complete

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  display.display();
  delay(2000);  
  display.clearDisplay();

  Serial.println("Adafruit MPU6050 test!");


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


  // Initialize display
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Ready!");
  display.display();
  delay(1000);
}


void loop() {
  // Get sensor readings
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  // Step Detection
  if ((az - previousZ) > stepThreshold) {
    stepCounter++;
    Serial.print("Step detected! Total steps: ");
    Serial.println(stepCounter);

    // Display step count on OLED
    display.clearDisplay();
    display.setTextSize(2);  
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Steps: ");
    display.println(stepCounter);
    display.display();
    delay(500);
  }


  // Fall Detection
  if (ax > fallThresholdHigh || ay > fallThresholdHigh || az > fallThresholdHigh || 
      ax < fallThresholdLow || ay < fallThresholdLow || az < fallThresholdLow) {
    // If no fall, keep monitoring status
    display.setTextSize(1);
    display.setCursor(0, 16);
    display.println("Monitoring...");
    display.display();
  
  } else {
     // Display fall detected on OLED
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("Fall");
    display.println("Detected!");
    display.display();
    Serial.println("Fall detected!"); 
  }

  previousZ = az;  // Update Z value for step detection

  delay(500);  // Delay for half a second
}