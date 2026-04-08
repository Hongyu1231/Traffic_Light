#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern String LTASpeedBandApiKey;
extern SemaphoreHandle_t g_trafficMutex;
extern SemaphoreHandle_t g_wifiMutex;
extern int g_speedBands[3];

// (请保留你之前的 extractValue 函数，放这里)
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
  
  // 👇【极度关键：声明在死循环外面】一生只申请一次内存
  WiFiClientSecure ltaClient;
  ltaClient.setInsecure();

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
        
        HTTPClient http;

        if (http.begin(ltaClient, "https://datamall2.mytransport.sg/ltaodataservice/v4/TrafficSpeedBands?$top=3")) {
          http.addHeader("AccountKey", LTASpeedBandApiKey);
          int httpCode = http.GET();
          
          if (httpCode == 200) {
            String payload = http.getString();
            Serial.println("\n[LTA] Fetch Success!");
            
            int found = 0, searchIndex = 0;
            while (found < 3) {
              int roadIndex = payload.indexOf("\"RoadName\":", searchIndex);
              if (roadIndex == -1) break;
              String speed = extractValue(payload, "SpeedBand", roadIndex);
              
              if (speed != "") {
                if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                  g_speedBands[found] = speed.toInt();
                  xSemaphoreGive(g_trafficMutex);
                }
                found++;
              }
              searchIndex = roadIndex + 1;
            }
          } else {
            Serial.printf("[LTA] Failed with HTTP Code: %d\n", httpCode);
          }
          http.end();
        }
        
        // 用完必须 stop 强行切断底层 TCP
        ltaClient.stop();
        xSemaphoreGive(g_wifiMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10000)); 
  }
}