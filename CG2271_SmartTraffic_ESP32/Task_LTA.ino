#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern String LTASpeedBandApiKey;
extern SemaphoreHandle_t g_trafficMutex;
extern SemaphoreHandle_t g_wifiMutex;
extern int g_speedBands[3];

// Helper parsing function to extract unquoted numbers or strings
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
  Serial.println("[LTA] Task is ALIVE (Unique Roads Filter Mode)");
  vTaskDelay(pdMS_TO_TICKS(5000)); 

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(30000)) == pdTRUE) {
        
        WiFiClientSecure ltaClient;
        ltaClient.setInsecure(); 
        HTTPClient http;

        String url = "https://datamall2.mytransport.sg/ltaodataservice/v4/TrafficSpeedBands?$top=30";
        
        if (http.begin(ltaClient, url)) {
          http.addHeader("AccountKey", LTASpeedBandApiKey);
          http.addHeader("accept", "application/json"); 
          
          int httpCode = http.GET();
          
          if (httpCode == 200) {
            String payload = http.getString();
            
            if (payload.length() > 0) {
              int found = 0, searchIndex = 0;
              int localSpeeds[3] = {0, 0, 0}; 
              String roadNames[3] = {"", "", ""}; 
              
              int valueStartIndex = payload.indexOf("\"value\":[");
              if (valueStartIndex != -1) searchIndex = valueStartIndex;

              while (found < 3) {
                int roadIndex = payload.indexOf("\"RoadName\":", searchIndex);
                if (roadIndex == -1) break; // Reached the end of the data
                
                String rName = extractValue(payload, "RoadName", roadIndex);
                String speedStr = extractValue(payload, "SpeedBand", roadIndex);
                
                if (speedStr != "" && rName != "") {
                  
                  // Check if this road name already exists in our array
                  bool isDuplicate = false;
                  for (int i = 0; i < found; i++) {
                    if (roadNames[i] == rName) {
                      isDuplicate = true;
                      break;
                    }
                  }

                  // Only add it to our display list if it is NOT a duplicate
                  if (!isDuplicate) {
                    roadNames[found] = rName;
                    localSpeeds[found] = speedStr.toInt();
                    found++; // Successfully found a unique road, increment counter
                  }
                }
                // Move the search index forward to find the next record
                searchIndex = roadIndex + 10; 
              }

              // Update the global array for the UART task safely
              if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                for(int i = 0; i < 3; i++) {
                  g_speedBands[i] = localSpeeds[i];
                }
                xSemaphoreGive(g_trafficMutex);
              }

              // Print the final 3 UNIQUE roads to the terminal
              Serial.println("\n====================================");
              Serial.println("📡 [LTA API] Live Traffic Speeds (Unique Roads):");
              for (int i = 0; i < found; i++) {
                Serial.printf("   ➡️ %s: Speedband %d\n", roadNames[i].c_str(), localSpeeds[i]);
              }
              Serial.println("====================================\n");

            }
          } else {
            Serial.printf("❌ [LTA] HTTP GET Failed, Code: %d\n", httpCode);
          }
          http.end();
        }
        
        ltaClient.stop(); 
        xSemaphoreGive(g_wifiMutex); 
        
      }
    }
    // Wait 15 seconds before fetching the next update
    vTaskDelay(pdMS_TO_TICKS(15000)); 
  }
}