#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern SemaphoreHandle_t g_wifiMutex;
extern volatile int g_mcxcValue;
extern volatile bool g_newMcxcData;

#define BOT_TOKEN "8698811083:AAHXeEDJ-GIRfNn9PhQUrjzBPa37KEylOt8"
#define CHAT_ID "7426320659"

void taskTelegramAlert(void* pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(5000)); 
  
  for (;;) {
    // Check if new speedband data has been validated by the UART task
    if (g_newMcxcData && WiFi.status() == WL_CONNECTED) {
      
      if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(20000)) == pdTRUE) {
        
        int band = g_mcxcValue;
        g_newMcxcData = false; 

        WiFiClientSecure teleClient;
        teleClient.setInsecure();
        HTTPClient http;

        // --- 🚨 Warning Logic: Trigger if Speedband > 7 ---
        String messageText;
        if (band > 7) {
          messageText = "🚨%20*TRAFFIC%20WARNING*%20🚨%0A";
          messageText += "High%20Speed%20Detected!%0A";
          messageText += "Current%20Speedband:%20" + String(band);
        } else {
          messageText = "Current%20Speedband:%20" + String(band);
        }

        // Send request using Markdown for bold text
        String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + 
                     "/sendMessage?chat_id=" + String(CHAT_ID) + 
                     "&parse_mode=Markdown&text=" + messageText;

        if (http.begin(teleClient, url)) {
          int httpCode = http.GET();
          if (httpCode == 200) {
            Serial.printf("✅ [Tele] Speedband update (%d) sent.\n", band);
          }
          http.end(); 
        } 
        
        teleClient.stop();
        xSemaphoreGive(g_wifiMutex);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Prevent overlapping SSL calls
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}