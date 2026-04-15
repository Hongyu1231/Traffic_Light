#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern SemaphoreHandle_t g_wifiMutex;
extern volatile int g_mcxcValue;
extern volatile bool g_newMcxcData;

#define BOT_TOKEN "8698811083:AAHXeEDJ-GIRfNn9PhQUrjzBPa37KEylOt8"
#define CHAT_ID "7426320659"

void taskTelegramAlert(void* pvParameters) {
  // Initial delay to allow system stabilization
  vTaskDelay(pdMS_TO_TICKS(5000)); 
  
  for (;;) {
    // Check for new validated data from the UART task
    if (g_newMcxcData) {
      
      int band = g_mcxcValue;
      g_newMcxcData = false; // Reset flag immediately to prevent missing future data

      // --- 🚨 Core Logic: Trigger alert ONLY if speed > 7 and WiFi is connected ---
      if (band > 7 && WiFi.status() == WL_CONNECTED) {
        
        // Take WiFi mutex ONLY when an alert needs to be sent (Optimizes performance)
        if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(20000)) == pdTRUE) {
          
          WiFiClientSecure teleClient;
          teleClient.setInsecure(); // Bypass SSL certificate validation for Telegram API
          HTTPClient http;

          // Build the warning message
          String messageText = "🚨%20*TRAFFIC%20WARNING*%20🚨%0A";
          messageText += "High%20Speed%20Detected!%0A";
          messageText += "Current%20Speedband:%20" + String(band);

          // Construct the Telegram API URL
          String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + 
                       "/sendMessage?chat_id=" + String(CHAT_ID) + 
                       "&parse_mode=Markdown&text=" + messageText;

          // Send the GET request to Telegram
          if (http.begin(teleClient, url)) {
            int httpCode = http.GET();
            if (httpCode == 200) {
              Serial.printf("✅ [Tele] Warning sent! Speedband: %d\n", band);
            } else {
              Serial.printf("❌ [Tele] Warning failed. HTTP Code: %d\n", httpCode);
            }
            http.end(); 
          } 
          
          teleClient.stop();
          xSemaphoreGive(g_wifiMutex); // Release WiFi lock
          
          // Add a buffer delay to prevent system overload from continuous high speed
          vTaskDelay(pdMS_TO_TICKS(2000)); 
        }
      }
    }
    // Task heartbeat check interval
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}