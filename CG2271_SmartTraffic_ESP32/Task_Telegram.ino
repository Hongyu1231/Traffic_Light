#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern SemaphoreHandle_t g_trafficMutex;
extern SemaphoreHandle_t g_wifiMutex;
extern int g_speedBands[3];

#define BOT_TOKEN "8698811083:AAHXeEDJ-GIRfNn9PhQUrjzBPa37KEylOt8"
#define CHAT_ID "7426320659"

void taskTelegramAlert(void* pvParameters) {
  
  // 👇【极度关键：声明在死循环外面】一生只申请一次内存，永不碎片化！
  WiFiClientSecure teleClient;
  teleClient.setInsecure();

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (xSemaphoreTake(g_wifiMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
        
        Serial.println("[Telegram] Attempting to send message via raw HTTP...");
        
        HTTPClient http;
        http.setConnectTimeout(5000);
        
        int localSpeeds[3] = {0, 0, 0};
        if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          for(int i=0; i<3; i++) localSpeeds[i] = g_speedBands[i];
          xSemaphoreGive(g_trafficMutex);
        }

        String msg = "Traffic+Report%0ARoad+1:+" + String(localSpeeds[0]) + "%0ARoad+2:+" + String(localSpeeds[1]) + "%0ARoad+3:+" + String(localSpeeds[2]);
        String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + msg;

        if (http.begin(teleClient, url)) {
          int httpCode = http.GET();
          if (httpCode == 200) {
            Serial.println("✅ [Telegram] Message Sent Successfully!");
          } else {
            Serial.printf("❌ [Telegram] API Error! HTTP Code: %d\n", httpCode);
          }
          http.end(); 
        } 
        
        // 用完必须 stop 强行切断底层 TCP，不留一点垃圾
        teleClient.stop(); 
        xSemaphoreGive(g_wifiMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}