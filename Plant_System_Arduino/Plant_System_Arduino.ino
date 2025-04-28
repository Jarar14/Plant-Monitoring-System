/*
 * For Plant Health Monitoring System
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverUrl = "http://your-server-url.com/api/plant-data";

// Function prototypes
void setupWiFi();
bool sendDataToServer(float soilMoisture, int lightLevel, float airQuality, bool isTilted, bool isCollision);

void setupWiFi() {
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi. Will try again later.");
  }
}

// Send data to server in JSON format; Output = TRUE if successful
bool sendDataToServer(float soilMoisture, int lightLevel, float airQuality, bool isTilted, bool isCollision) {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    setupWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      return false;
    }
  }
  
  // Create JSON document
  StaticJsonDocument<200> doc;
  doc["soil_moisture"] = soilMoisture;
  doc["light_level"] = lightLevel;
  doc["air_quality"] = airQuality;
  doc["is_tilted"] = isTilted;
  doc["is_collision"] = isCollision;
  
  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send HTTP POST request
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
    http.end();
    return true;
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

void readSensors(){
  //
}

void processData(){
  //
}

void loop() {
  // Read all sensor data
  readSensors();
  
  // Process data and decide on alert status
  processData();
  
  // Send data to server every X minutes
  int interval = 5;   // 5 minutes
  static unsigned long lastUploadTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastUploadTime >= interval * 60 * 1000) {
    lastUploadTime = currentTime;
    
    bool uploadSuccess = sendDataToServer(
      soilHumidity, 
      lightLevel, 
      airQuality, 
      isPlantTilted, 
      isCollision
    );
    
    if (uploadSuccess) {
      Serial.println("Data uploaded successfully");
    } else {
      Serial.println("Failed to upload data");
    }
  }
}