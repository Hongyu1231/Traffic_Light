#include <Arduino.h>

extern SemaphoreHandle_t g_trafficMutex;
extern int g_speedBands[3];
extern volatile int g_rfidUartFlag;

extern volatile int g_mcxcValue;
extern volatile bool g_newMcxcData;

void taskUARTComm(void* pvParameters) {
  uint32_t lastTx = 0;
  Serial1.setTimeout(50);

  for (;;) {
    // --- 📥 RX Logic: Check for incoming data from MCXC444 ---
    while (Serial1.available() > 0) {
      String rxStr = Serial1.readStringUntil('\n');
      rxStr.trim();

      if (rxStr.length() > 0) {
        int tempVal = rxStr.toInt();
        
        // 💡 New Validation: Range 0 to 9 for Speedbands
        if (tempVal >= 0 && tempVal <= 9) {
          g_mcxcValue = tempVal;
          g_newMcxcData = true; // Trigger Telegram update
          Serial.printf("[UART RX] Valid Speedband Received: %d\n", g_mcxcValue);
        } else {
          Serial.printf("[UART RX] Invalid Speedband Range: %s\n", rxStr.c_str());
        }
      }
    }

    // --- 📤 TX Logic: Broadcast to MCXC444 every 5 seconds ---
    if (millis() - lastTx >= 5000) {
      int localSpeeds[3] = {0, 0, 0};
      int rfidStatus = g_rfidUartFlag;

      // Safely access shared traffic data from LTA task
      if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (int i = 0; i < 3; i++) localSpeeds[i] = g_speedBands[i];
        xSemaphoreGive(g_trafficMutex);
      }

      // Send to MCXC444 in format: s1,s2,s3,rfid\n
      Serial1.printf("%d,%d,%d,%d\n",
                     localSpeeds[0],
                     localSpeeds[1],
                     localSpeeds[2],
                     rfidStatus);

      // Reset RFID flag after successful transmission
      if (rfidStatus == 1) g_rfidUartFlag = 0;

      Serial.printf("[UART TX] Broadcast Sent: %d,%d,%d,%d\n",
                    localSpeeds[0],
                    localSpeeds[1],
                    localSpeeds[2],
                    rfidStatus);

      lastTx = millis();
    }

    // Task heartbeat: check RX buffer every 50ms
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}