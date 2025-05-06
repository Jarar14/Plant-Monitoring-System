 /*
 * For Plant Health Monitoring System
 * Team 1 - Fixed MPU6050 Connection
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>

// MPU6050 Gyroscope
#define SDA_PIN   21
#define SCL_PIN   22
#define MPU_ADDR  0x68  // AD0 pin connected to GND
#define MPU_PWR_MGMT_1   0x6B
#define MPU_ACCEL_XOUT_H 0x3B
int16_t accelX, accelY, accelZ;
bool isPlantTilted = false;

// Air Quality + Humidity
int sensorValue;
int soilsensorValue;
#define GAS_PIN       2
#define HUMIDITY_PIN  4

// LED pins
#define redPin    16
#define bluePin   2
#define greenPin  18

// Light Sensor
#define lightSensorPin 34   // Analog pin for LDR
#define greenLEDPin 25      // Green LED
int lightLevel = 0;

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverUrl = "http://your-server-url.com/api/plant-data";

// Function prototype for WiFi setup
void setupWiFi();

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial monitor time to start
  Serial.println("Plant Health Monitoring System Starting...");
  pinMode(GAS_PIN, INPUT);
  
  // Initialize MPU6050 I2C connection
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

  // Light sensor setup
  pinMode(lightSensorPin, INPUT);
  pinMode(greenLEDPin, OUTPUT);
  digitalWrite(greenLEDPin, LOW);
}

// Initialize the MPU6050
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
  
  // Wake up the MPU6050 (Essential)
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
  Wire.write(MPU_ACCEL_XOUT_H);  // Register to start reading
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
    
    // Convert to g forces (+-2g range by default)
    float aX = accelX / 16384.0;
    float aY = accelY / 16384.0;
    float aZ = accelZ / 16384.0;
    
    /*
    // Debug output
    Serial.print("Accel X: "); Serial.print(aX, 2);
    Serial.print(" | Y: "); Serial.print(aY, 2);
    Serial.print(" | Z: "); Serial.println(aZ, 2);
    */
    
    // Calculate tilt
    float tiltMagnitude = sqrt(aX*aX + aY*aY);
    isPlantTilted = (tiltMagnitude > 0.3);
    
    return true;
  } else {
    Serial.println("Failed to read all accelerometer data!");
    return false;
  }
}

// Read Light Sensor
void readLightSensor() {
  int rawValue = analogRead(lightSensorPin);
  lightLevel = map(rawValue, 0, 4095, 0, 100);  // Mapping to percentage (0 = low light, 100 = max light)

  //Serial.print("Raw Light Value: ");
  //Serial.print(rawValue);
  Serial.print(" => Light Level (%): ");
  Serial.println(lightLevel);
}

// Determine Plant Light Condition
void processLightData() {
  if (lightLevel < 30) {
    digitalWrite(greenLEDPin, LOW);   // Green LED OFF
    Serial.println("Low Light Detected!");
  } else {
    digitalWrite(greenLEDPin, HIGH);  // Green LED ON
    Serial.println("Normal Light");
  }
  
}

void loop() {
  JSONVar data;
  WiFi.begin(ssid,password)
  WiFiClient client;
  HTTPClient http;
  http.begin(client, server);
  http.addHeader("Content-Type", "application/json");

  readLightSensor();
  processLightData();

  int airValue = analogRead(GAS_PIN);
  int humidityValue = analogRead(HUMIDITY_PIN);
  Serial.print("Air Quality Value: ");
  Serial.println(airValue, DEC);
  Serial.print("Soil Humidity Value: ");
  Serial.println(humidityValue, DEC);
  
  // Attempt to read accelerometer data
  bool readSuccess = readAccelerometer();
  
  delay(100);
  digitalWrite(bluePin, LOW);
  digitalWrite(redPin, LOW);
  
  // Process plant tilt status
  Serial.print("Plant tilted: ");
  Serial.println(isPlantTilted ? "YES" : "NO");
  
  data["team_number"] = 1;
  data["Light Level"] = lightLevel;
  data["Air Quality"] = airValue;
  data["Soil Humidity"] = humidityValue;

  msg = JSON.stringify(encryptedData);

  int responseCode = http.POST(msg);
  Serial.println(msg);

  // Wait before next reading
  delay(2000);
}