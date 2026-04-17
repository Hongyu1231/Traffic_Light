#include <HTTPClient.h>
#include <WiFi.h>

extern SemaphoreHandle_t g_wifiMutex;
extern volatile int g_mcxcValue;
extern volatile bool g_uploadDbFlag;

const char* serverUrl = "http://192.168.43.189:3000/api/telemetry"; // Update with your IP
String currentSessionId;

void taskDatabaseUpload(void* pvParameters) {
  // Generate a unique session ID once per reboot
  randomSeed(analogRead(0));
  currentSessionId = "Sess_" + String(random(1000, 9999));
  
  for (;;) {
    if (g_uploadDbFlag && WiFi.status() == WL_CONNECTED) {
      if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
        
        int speedToUpload = g_mcxcValue;
        g_uploadDbFlag = false; 

        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String payload = "{\"sessionId\":\"" + currentSessionId + "\", \"speedrange\":" + String(speedToUpload) + "}";
        int httpCode = http.POST(payload);
        
        if (httpCode == 200) {
          Serial.printf("[DB] Uploaded Speed: %d\n", speedToUpload);
        } else {
          Serial.printf("[DB] Post Failed: %d\n", httpCode);
        }
        
        http.end();
        xSemaphoreGive(g_wifiMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}