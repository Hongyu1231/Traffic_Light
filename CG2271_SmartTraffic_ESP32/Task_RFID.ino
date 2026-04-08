#include <SPI.h>
#include <MFRC522.h>

// --- Hardware Pin Definitions for MFRC522 ---
#define RST_PIN 9
#define SS_PIN  10
#define SCK_PIN 12
#define MISO_PIN 13
#define MOSI_PIN 11

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Reference to the global flag used for UART communication
extern volatile int g_rfidUartFlag;

void taskPollRFID(void* pvParameters) {
  // Initialize SPI bus and the RFID reader
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();
  
  // The specific UID authorized for "VIP" access
  const String targetUID = "534C0AA0005980";
  Serial.println("✅ [RFID] Scanner Active.");

  for (;;) {
    // Check if a new card is present and can be read
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String rfidUID = "";
        
        // Construct the UID string from the hex bytes
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          rfidUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
          rfidUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        rfidUID.toUpperCase();

        // Compare scanned UID with the authorized target
        if (rfidUID == targetUID) {
          Serial.println("[RFID] 🟢 VIP identified");
          // Signal the UART task to include the detection in the next broadcast
          g_rfidUartFlag = 1; 
        } else {
          Serial.printf("[RFID] 🔴 Unauthorised: %s\n", rfidUID.c_str());
        }

        // Halt the current card and stop encryption to prepare for the next scan
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        
        // Cooldown period to prevent multiple triggers from a single tap
        vTaskDelay(pdMS_TO_TICKS(1500)); 
    }
    
    // Short polling delay to keep the task responsive without hogging the CPU
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}