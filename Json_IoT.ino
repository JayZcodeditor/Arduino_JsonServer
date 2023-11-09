#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

float temp;
float hum;
unsigned long count = 1;

const char* ssid = "JXYZ";
const char* password = "313326339";

WiFiClient client;
HTTPClient http;
DHT dht11(D4, DHT11);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60 * 60 * 7); // Set the time zone offset (Thailand: UTC+7)

// Thailand local time offset from UTC
const int thailandLocalTimeOffset = 7 * 3600;

String getCurrentTimestamp() {
  timeClient.update();
  time_t now = timeClient.getEpochTime() + thailandLocalTimeOffset;
  struct tm* timeInfo = localtime(&now);

  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%d-%m-%Y %H:%M:%S", timeInfo);
  return String(timestamp);
}

void ReadDHT11() {
  temp = dht11.readTemperature();
  hum = dht11.readHumidity();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  dht11.begin();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  // Initialize NTP client
  timeClient.begin();
  timeClient.update();
}

void loop() {
  static unsigned long lastTime = 0;
  unsigned long timerDelay = 15000;

  if ((millis() - lastTime) > timerDelay) {
    ReadDHT11();
    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor");
    } else {
      String timestamp = getCurrentTimestamp();

      Serial.print("Humidity: ");
      Serial.println(hum);
      Serial.print("Temperature: ");
      Serial.println(temp);
      Serial.print("Timestamp: ");
      Serial.println(timestamp);

      StaticJsonDocument<200> jsonDocument;
      jsonDocument["timestamp"] = timestamp;
      jsonDocument["humidity"] = hum;
      jsonDocument["temperature"] = temp;


      String jsonData;
      serializeJson(jsonDocument, jsonData);

      http.begin(client, "http://172.20.10.3:3000/sensors");
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        String payload = http.getString();
        Serial.println("Returned payload:");
        Serial.println(payload);
        count += 1;
      } else {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
      }
      http.end();
    }
    lastTime = millis();
  }
}
