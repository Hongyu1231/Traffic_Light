#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern String LTASpeedBandApiKey;
extern SemaphoreHandle_t g_trafficMutex;
extern SemaphoreHandle_t g_wifiMutex;
extern int g_speedBands[3];

// Helper function to extract specific values from the JSON response string
String extractValue(const String& json, const String& key, int fromIndex) {
  int index = json.indexOf("\"" + key + "\":", fromIndex);
  if (index == -1) return "";
  int start = index + key.length() + 3;
  if (json[start] == '\"') {
    start++;
    int end = json.indexOf("\"", start);
    return json.substring(start, end);
  } else {
    int end = json.indexOf(",", start);
    if (end == -1) end = json.indexOf("}", start);
    return json.substring(start, end);
  }
}

void taskLTA_Pipeline(void* pvParameters) {
  Serial.println("🚩 [LTA] Task is ALIVE and starting...");
  
  // Wait 5 seconds after boot to avoid the initial WiFi connection surge
  vTaskDelay(pdMS_TO_TICKS(5000)); 

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[LTA] Waiting for WiFi Mutex...");
      
      // Attempt to take the WiFi Mutex with a 30-second timeout
      if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(30000)) == pdTRUE) {
        Serial.println("[LTA] Mutex Taken! Starting SSL Connection...");
        
        WiFiClientSecure ltaClient;
        ltaClient.setInsecure(); // Skip SSL certificate validation
        ltaClient.setTimeout(15000);

        HTTPClient http;
        http.setTimeout(15000);

        // Append a millis timestamp to the URL to prevent API caching
        String url = "https://datamall2.mytransport.sg/ltaodataservice/v4/TrafficSpeedBands?$top=3&t=" + String(millis());
        
        if (http.begin(ltaClient, url)) {
          http.addHeader("AccountKey", LTASpeedBandApiKey);
          Serial.println("[LTA] Sending GET Request...");
          
          int httpCode = http.GET();
          Serial.printf("[LTA] Server responded with: %d\n", httpCode);
          
          if (httpCode == 200) {
            String payload = http.getString();
            if (payload.length() > 0) {
              Serial.printf("✅ [LTA] Received %d bytes. Parsing...\n", payload.length());
              int found = 0, searchIndex = 0;
              
              while (found < 3) {
                int roadIndex = payload.indexOf("\"RoadName\":", searchIndex);
                if (roadIndex == -1) break;
                
                String speed = extractValue(payload, "SpeedBand", roadIndex);
                if (speed != "") {
                  // Safely update the shared global speed array
                  if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_speedBands[found] = speed.toInt();
                    xSemaphoreGive(g_trafficMutex);
                  }
                  found++;
                }
                searchIndex = roadIndex + 1;
              }
            }
          } else {
            Serial.println("❌ [LTA] HTTP GET Failed");
          }
          http.end();
        }
        
        ltaClient.stop(); // Properly close the SSL connection
        Serial.println("[LTA] Releasing Mutex and sleeping...");
        xSemaphoreGive(g_wifiMutex);
        
        // Critical: Allow a 5s window for other tasks (like Telegram) to use the WiFi
        vTaskDelay(pdMS_TO_TICKS(5000));
        
      } else {
        Serial.println("⚠️ [LTA] Could not get Mutex for 30s!");
      }
    }
    // Poll the LTA server every 20 seconds
    vTaskDelay(pdMS_TO_TICKS(20000)); 
  }
}