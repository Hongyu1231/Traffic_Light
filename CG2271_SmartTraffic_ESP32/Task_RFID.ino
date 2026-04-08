#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN  10
#define SCK_PIN 12
#define MISO_PIN 13
#define MOSI_PIN 11

MFRC522 mfrc522(SS_PIN, RST_PIN);

// 👇 引入刚才在主程序定义的标记
extern volatile int g_rfidUartFlag;

void taskPollRFID(void* pvParameters) {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();
  
  const String targetUID = "534C0AA0005980";
  Serial.println("\n✅ [RFID] Scanner Active...");

  for (;;) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String rfidUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          rfidUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
          rfidUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        rfidUID.toUpperCase();

        if (rfidUID == targetUID) {
          Serial.println("[RFID] 🟢 VIP identified");
          // 👇 识别成功，通知 UART 任务
          g_rfidUartFlag = 1; 
        } else {
          Serial.printf("[RFID] 🔴 unauthorised (Scanned ID: %s)\n", rfidUID.c_str());
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        
        vTaskDelay(pdMS_TO_TICKS(1500)); 
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}