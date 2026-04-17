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
  Serial.println("[RFID] Scanner Active.");

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

        // MODIFICATION 1: Assign 2 or 1 based on UID validation
        if (rfidUID == targetUID) {
          Serial.println("[RFID] VIP identified (Sending 2)");
          g_rfidUartFlag = 2; // Verified
        } else {
          Serial.printf("[RFID] Unauthorised: %s (Sending 1)\n", rfidUID.c_str());
          g_rfidUartFlag = 1; // Unverified
        }

        // Halt the current card and stop encryption to prepare for the next scan
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        
        // Hold the '1' or '2' state for 1.5 seconds. 
        // This acts as a signal pulse, giving the UART task enough time to read and transmit it.
        vTaskDelay(pdMS_TO_TICKS(1500)); 

        g_rfidUartFlag = 0; 
    }
    
    // Short polling delay to keep the task responsive without hogging the CPU
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}