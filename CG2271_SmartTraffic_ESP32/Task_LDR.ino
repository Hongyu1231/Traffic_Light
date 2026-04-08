#include <Arduino.h>

// --- Extern Linkage ---
// This tells this tab that these variables are defined in the main tab
extern int g_ldrValue;
extern SemaphoreHandle_t g_ldrMutex;

// The LDR_PIN must be defined in the main tab or here. 
// If it's defined as a #define in the main tab, we define it here too if needed.
#ifndef LDR_PIN
  #define LDR_PIN 1  // Default to Pin 1 if not found
#endif

void taskPollLDR(void* pvParameters) {
  for (;;) {
    // 1. Read raw ADC value (0 - 4095 for ESP32-S2)
    int raw = analogRead(LDR_PIN);       

    // 2. Apply logic inversion: Higher value = Brighter light
    // (Assuming LDR is in a divider where voltage drops as it gets darker)
    int invertedValue = 4095 - raw;

    // 3. Convert to a clean 0-100% scale
    int lightPercent = (int)(((float)invertedValue / 4095.0f) * 100.0f);

    // 4. Safely update the global variable using the Mutex
    if (xSemaphoreTake(g_ldrMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      g_ldrValue = lightPercent;
      xSemaphoreGive(g_ldrMutex);
    }

    // Optional: Debug print (uncomment if you have Serial connected)
    // Serial.printf("[LDR] Intensity: %d%%\n", lightPercent);

    // 5. Sample every 500ms
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}