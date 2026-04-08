#include <Arduino.h>

extern SemaphoreHandle_t g_trafficMutex;
extern int g_speedBands[3];
extern volatile int g_rfidUartFlag; // 引入标记

void taskUARTComm(void* pvParameters) {
  Serial.println("[UART] UART task started");
  int sendCount = 0;

  for (;;) {
    int localSpeeds[3] = {0, 0, 0};
    int rfidStatus = g_rfidUartFlag; 

    if (g_trafficMutex != NULL) {
      if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for(int i=0; i<3; i++) localSpeeds[i] = g_speedBands[i];
        xSemaphoreGive(g_trafficMutex);
      }
    }

    // --- 强制发送 4 个变量 ---
    Serial1.print("SPEEDS:");
    Serial1.print(localSpeeds[0]);
    Serial1.print(",");
    Serial1.print(localSpeeds[1]);
    Serial1.print(",");
    Serial1.print(localSpeeds[2]);
    Serial1.print(",");
    Serial1.println(rfidStatus); // 最后这个是 0 或 1

    // 发了 1 之后立刻复位
    if (rfidStatus == 1) {
      g_rfidUartFlag = 0;
    }

    sendCount++;
    // 强制更新控制台打印的格式，带上 rfidStatus
    Serial.printf("[UART TX] Sent #%d: SPEEDS:%d,%d,%d,%d\n", sendCount, localSpeeds[0], localSpeeds[1], localSpeeds[2], rfidStatus);

    vTaskDelay(pdMS_TO_TICKS(5000)); 
  }
}