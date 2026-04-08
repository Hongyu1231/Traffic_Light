// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// #define SCREEN_WIDTH 128 
// #define SCREEN_HEIGHT 64 
// #define OLED_RESET    -1 
// #define OLED_SDA 6
// #define OLED_SCL 7

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// extern SemaphoreHandle_t g_trafficMutex;
// extern int g_speedBands[3];
// // 引入全局状态变量
// extern volatile int g_rfidState;

// void taskDisplayOLED(void* pvParameters) {
//   Wire.begin(OLED_SDA, OLED_SCL);
//   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
//     Serial.println(F("❌ [OLED] 初始化失败！"));
//     vTaskDelete(NULL); 
//   }

//   for (;;) {
//     int localSpeeds[3] = {0, 0, 0};

//     if (g_trafficMutex != NULL) {
//       if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
//         for(int i=0; i<3; i++) localSpeeds[i] = g_speedBands[i];
//         xSemaphoreGive(g_trafficMutex);
//       }
//     }

//     display.clearDisplay();      
//     display.setTextSize(1);      
//     display.setTextColor(SSD1306_WHITE); 
    
//     // 绘制 LTA 交通数据
//     display.setCursor(0, 0);     
//     display.println(F("=== SMART TRAFFIC ==="));
//     display.setCursor(0, 15);
//     display.print(F("Road 1 Band: ")); display.println(localSpeeds[0]);
//     display.setCursor(0, 25);
//     display.print(F("Road 2 Band: ")); display.println(localSpeeds[1]);
//     display.setCursor(0, 35);
//     display.print(F("Road 3 Band: ")); display.println(localSpeeds[2]);

//     // --- 动态绘制 RFID 状态 ---
//     display.setCursor(0, 50);
//     if (g_rfidState == 0) {
//       display.println(F("RFID: Scanning..."));
//     } 
//     else if (g_rfidState == 1) {
//       // 身份正确时，采用黑底白字高亮反转！
//       display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
//       display.println(F(">> VIP IDENTIFIED <<"));
//       display.setTextColor(SSD1306_WHITE); // 恢复正常颜色
//     } 
//     else if (g_rfidState == 2) {
//       display.println(F("! UNAUTHORISED !"));
//     }

//     display.display();           
    
//     // OLED 刷新率提升到 100ms，这样刷卡瞬间屏幕就会立刻反应
//     vTaskDelay(pdMS_TO_TICKS(100)); 
//   }
// }