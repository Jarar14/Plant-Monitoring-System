/*
 * For Plant Health Monitoring System
 * Team 1 - Fixed MPU6050 Connection
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>

// MPU6050 Gyroscope
#define SDA_PIN 21
#define SCL_PIN 22
#define MPU_ADDR 0x68  // AD0 pin connected to GND
#define MPU_PWR_MGMT_1 0x6B
#define MPU_ACCEL_XOUT_H 0x3B

// Other sensor variables
int16_t accelX, accelY, accelZ;
bool isPlantTilted = false;

// LED pins
int redPin = 16;
int bluePin = 2;
int greenPin = 18;

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverUrl = "http://your-server-url.com/api/plant-data";

// Function prototype for WiFi setup
void setupWiFi();

void setup() {
  Serial.begin(115200);  // Increased baud rate for better debugging
  delay(1000);  // Give serial monitor time to start
  
  Serial.println("Plant Health Monitoring System Starting...");
  
  // Initialize MPU6050 I2C connection with proper settings
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Enable internal pull-up resistors
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  
  // Try to connect to MPU6050
  bool mpuInitialized = initMPU6050();
  if (mpuInitialized) {
    Serial.println("MPU6050 initialized successfully!");
  } else {
    Serial.println("FAILED to initialize MPU6050! Check connections.");
  }
  
  // Initialize LED pins
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  
  // Turn on green LED to show setup completed
  digitalWrite(greenPin, HIGH);
  digitalWrite(redPin, LOW);
  digitalWrite(bluePin, LOW);
}

// New function to properly initialize the MPU6050
bool initMPU6050() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Try to communicate with MPU6050
  Wire.beginTransmission(MPU_ADDR);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.print("Error connecting to MPU6050: ");
    Serial.println(error);
    return false;
  }
  
  // Wake up the MPU6050 - essential step!
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(MPU_PWR_MGMT_1);  // Power Management register
  Wire.write(0);               // Set to zero to wake up
  error = Wire.endTransmission(true);
  
  if (error != 0) {
    Serial.print("Error waking up MPU6050: ");
    Serial.println(error);
    return false;
  }
  
  delay(100);  // Give MPU6050 time to wake up
  return true;
}

// Improved function to read data from the accelerometer
bool readAccelerometer() {
  // Send request to read accelerometer data
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(MPU_ACCEL_XOUT_H);  // Register to start reading from
  byte error = Wire.endTransmission(false);
  
  if (error != 0) {
    Serial.print("Error requesting accelerometer data: ");
    Serial.println(error);
    return false;
  }
  
  // Request 6 bytes (X, Y, Z accelerometer values)
  Wire.requestFrom(MPU_ADDR, 6, true);
  
  // Check if we received the expected number of bytes
  if (Wire.available() == 6) {
    accelX = Wire.read() << 8 | Wire.read();
    accelY = Wire.read() << 8 | Wire.read();
    accelZ = Wire.read() << 8 | Wire.read();
    
    // Convert to g forces (±2g range by default)
    float aX = accelX / 16384.0;
    float aY = accelY / 16384.0;
    float aZ = accelZ / 16384.0;
    
    // Debug output
    Serial.print("Accel X: "); Serial.print(aX, 2);
    Serial.print(" | Y: "); Serial.print(aY, 2);
    Serial.print(" | Z: "); Serial.println(aZ, 2);
    
    // Calculate tilt
    float tiltMagnitude = sqrt(aX*aX + aY*aY);
    isPlantTilted = (tiltMagnitude > 0.2);
    
    return true;
  } else {
    Serial.println("Failed to read all accelerometer data!");
    return false;
  }
}

// I2C Scanner to help diagnose connection issues
void scanI2CDevices() {
  byte error, address;
  int deviceCount = 0;
  
  Serial.println("Scanning for I2C devices...");
  
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      deviceCount++;
      
      if (address == MPU_ADDR) {
        Serial.println("MPU6050 found at expected address!");
      }
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (deviceCount == 0) {
    Serial.println("No I2C devices found! Check your connections.");
  } else {
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" device(s).");
  }
}

void loop() {
  // Scan I2C devices on first run or periodically to help with debugging
  //remove if sensors running consistently
  static bool firstRun = true;
  if (firstRun) {
    scanI2CDevices();
    firstRun = false;
  }
  
  // Attempt to read accelerometer data
  bool readSuccess = readAccelerometer();
  
  // Visual indicator - blink blue LED on successful read
  if (readSuccess) {
    digitalWrite(bluePin, HIGH);
  } else {
    digitalWrite(redPin, HIGH);  // Red LED for errors
  }
  
  delay(100);
  digitalWrite(bluePin, LOW);
  digitalWrite(redPin, LOW);
  
  // Process plant tilt status
  Serial.print("Plant tilted: ");
  Serial.println(isPlantTilted ? "YES" : "NO");
  
  // Wait before next reading
  delay(1000);
}