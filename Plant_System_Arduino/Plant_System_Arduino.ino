 /*
 * For Plant Health Monitoring System
 * Team 1 - Fixed MPU6050 Connection
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>
#include <Crypto.h>
#include <AES.h>

// Encryption
byte key[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
byte cipher[80];
AES128 aes128;

// MPU6050 Gyroscope
#define SDA_PIN   21
#define SCL_PIN   22
#define MPU_ADDR  0x68  // AD0 pin connected to GND
#define MPU_PWR_MGMT_1   0x6B
#define MPU_ACCEL_XOUT_H 0x3B
int16_t accelX, accelY, accelZ;
bool isPlantTilted = false;
float tiltMagnitude;

// Air Quality + Humidity
int sensorValue;
int soilsensorValue;
#define GAS_PIN       32
#define HUMIDITY_PIN  35

// Light Sensor
int lightLevel = 0;
#define lightSensorPin 34   // Analog pin for LDR

// LED pins
#define redPin    25
#define bluePin   26
#define greenPin  27
int blueValue;
int redValue;
int greenValue;

// WiFi credentials
const char* ssid = "Group1";
const char* password = "groupgroup1";

// Server details
const char* serverUrl = "http://192.168.195.107:5000/api/update";

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial monitor time to start
  Serial.println("Plant Health Monitoring System Starting...");

  WiFi.begin(ssid,password);


  pinMode(GAS_PIN, INPUT);
  
  // Encryption setup
  aes128.setKey(key,16);

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
  
  digitalWrite(greenPin, 1);
  digitalWrite(redPin, 1);
  digitalWrite(bluePin, 1);

  // Light sensor setup
  pinMode(lightSensorPin, INPUT);
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

// Function to read data from the accelerometer
float readAccelerometer() {
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
    
    // Calculate tilt
    float tiltMagnitude = sqrt(aX*aX + aY*aY);
    
    return tiltMagnitude;
  } else {
    Serial.println("Failed to read all accelerometer data!");
    return tiltMagnitude;
  }
}

// Read Light Sensor
int readLightSensor() {
  int rawValue = analogRead(lightSensorPin);
  lightLevel = 100 - map(rawValue, 0, 4095, 0, 100);  // Mapping to percentage (0 = low light, 100 = max light)

  Serial.print(" => Light Level (%): ");
  Serial.println(lightLevel);

  return lightLevel;
}

void determineLED(int humidityValue, int airValue, int lightValue, bool isPlantTilted) {
  // First turn off all LEDs to start with a clean state
  digitalWrite(redPin, 1);
  digitalWrite(bluePin, 1);
  digitalWrite(greenPin, 1);
  
  // Static variables for toggling
  static int  tiltColorState = 0;
  static bool toggleRedBlue = false;
  static bool toggleRed = false;
  static bool toggleBlue = false;
  
  // Determine state and set LEDs based on priority
  if (isPlantTilted) {
    // Cycle through colors for tilt alert
    tiltColorState = (tiltColorState + 1) % 3;
    
    if (tiltColorState == 0) {
      digitalWrite(redPin, 0);
    } 
    else if (tiltColorState == 1) {
      digitalWrite(bluePin, 0);
    }
    else {
      digitalWrite(greenPin, 0);
    }
    Serial.println("Plant Knocked Over!");
  }
  else if (lightValue < 75 && humidityValue < 30) {
    // Alternate Red and Blue for low light & humidity
    toggleRedBlue = !toggleRedBlue;
    
    if (toggleRedBlue) {
      digitalWrite(redPin, 0);
    } else {
      digitalWrite(bluePin, 0);
    }
    Serial.println("Low Light & Humidity!");
  }
  else if (lightValue < 75) {
    // Toggle Blue for low light
    toggleBlue = !toggleBlue;
    digitalWrite(bluePin, toggleBlue);
    digitalWrite(redPin, 1);
    digitalWrite(greenPin, 1);

    Serial.println("Low Light!");
  }
  else if (humidityValue < 30) {
    // Toggle ONLY Red for low humidity
    toggleRed = !toggleRed;
    digitalWrite(redPin, toggleRed);
    digitalWrite(greenPin, 1);
    digitalWrite(bluePin, 1);
    Serial.println("Low Humidity!");
  }
  else if (airValue > 3000) {
    // Purple for bad air (red + blue)
    digitalWrite(redPin, 0);
    digitalWrite(bluePin, 0);
    digitalWrite(greenPin, 1);
    Serial.println("Bad Air Quality!");
  }
  else {
    Serial.println("ALL GOOD!");
  }
}

void loop() {

  int lightValue = readLightSensor();
  int airValue = analogRead(GAS_PIN);
  int humidityValue = analogRead(HUMIDITY_PIN);

  Serial.print("Air Quality Value: ");
  Serial.println(airValue, DEC);
  Serial.print("Soil Humidity Value: ");
  Serial.println(humidityValue, DEC);
  
  // Attempt to read accelerometer data
  tiltMagnitude = readAccelerometer();
  isPlantTilted = (tiltMagnitude > 0.8);
  Serial.println(tiltMagnitude);

  // Process plant tilt status
  Serial.print("Plant tilted: ");
  Serial.println(isPlantTilted ? "YES" : "NO"); 

  //LED state determination
  determineLED(humidityValue, airValue, lightValue, isPlantTilted);

  //JSON Packet Setup
  JSONVar data;
  JSONVar encryptedData;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/json");
  
  data["air"] = airValue;
  data["soil"] = humidityValue;
  data["light"] = lightLevel;
  data["tilt"] = tiltMagnitude;
  data["knocked"] = isPlantTilted;

  String msg = JSON.stringify(data);

  // copy JSON message to plaintext buffer for encryption
  byte plaintext[msg.length()];
  for(int i = 0; i < msg.length(); i++){
    plaintext[i] = msg[i];
  }

  // Encrypt all blocks
  aes128.encryptBlock(cipher,plaintext);
  aes128.encryptBlock(&cipher[16],&plaintext[16]);
  aes128.encryptBlock(&cipher[32],&plaintext[32]);
  aes128.encryptBlock(&cipher[48],&plaintext[48]);
  aes128.encryptBlock(&cipher[64],&plaintext[64]);

  // Create ciphertext String to be sent in JSON
  String bytestream;
  for(int j = 0; j < sizeof(cipher); j++){
    bytestream += cipher[j];
    bytestream+= " ";
  }

  // Load encrypted JSON
  encryptedData["cipher"] = bytestream;
  msg = JSON.stringify(encryptedData);

  int responseCode = http.POST(msg);
  Serial.println(msg);
  
  // Wait before next reading
  delay(1000);
}