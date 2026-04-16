#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SDA 4
#define I2C_SCL 5

LiquidCrystal_I2C lcd(0x27, 20, 4);

extern volatile int g_rfidUartFlag;

// --- State Machine Variables ---
enum LCDState { STATE_WELCOME, STATE_PARKING };
LCDState currentState = STATE_WELCOME;
unsigned long displayStartTime = 0;
const unsigned long DISPLAY_DURATION = 10000; // 10 seconds in milliseconds

void taskDisplayLCD(void* pvParameters) {
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();

  // Initial UI
  lcd.setCursor(0, 0);
  lcd.print("=== SMART PARKING ===");
  lcd.setCursor(0, 1);
  lcd.print("      WELCOME       ");

  for (;;) {
    unsigned long currentTime = millis();

    switch (currentState) {
      case STATE_WELCOME:
        // 检查是否有新的 RFID 触发
        if (g_rfidUartFlag == 1) {
          // 切换到停车显示状态
          currentState = STATE_PARKING;
          displayStartTime = currentTime; // 记录开始时间
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("=== VIP ACCESS ===");
          lcd.setCursor(0, 1);
          lcd.print("Please enter the");
          lcd.setCursor(0, 2);
          lcd.print("parking lot now.");
        }
        break;

      case STATE_PARKING:
        // 检查是否已经过了 10 秒
        if (currentTime - displayStartTime >= DISPLAY_DURATION) {
          // 时间到，切回 Welcome 状态
          currentState = STATE_WELCOME;
          g_rfidUartFlag = 0; // 重置标志位，等待下次刷卡
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("=== SMART PARKING ===");
          lcd.setCursor(0, 1);
          lcd.print("      WELCOME       ");
        }
        break;
    }

    // 任务刷新频率，不需要太快，200ms 足够
    vTaskDelay(pdMS_TO_TICKS(200)); 
  }
}