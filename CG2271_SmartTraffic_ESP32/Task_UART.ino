#include <Arduino.h>

extern SemaphoreHandle_t g_trafficMutex;
extern int g_speedBands[3];
extern volatile int g_rfidUartFlag;
extern volatile bool g_uploadDbFlag;

extern volatile int g_mcxcValue;
extern volatile bool g_newMcxcData;

static const char UART_KEY[] = "CG2271_UART_KEY";

static char toHexNibble(uint8_t value) {
  value &= 0x0F;
  return value < 10 ? (char)('0' + value) : (char)('A' + value - 10);
}

static int fromHexNibble(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  return -1;
}

static String makeSecurePacket(const String& plainText) {
  String cipherHex = "";
  uint8_t checksum = 0;
  size_t keyLen = strlen(UART_KEY);

  cipherHex.reserve(plainText.length() * 2);

  for (size_t i = 0; i < plainText.length(); i++) {
    uint8_t encrypted = ((uint8_t)plainText[i]) ^ ((uint8_t)UART_KEY[i % keyLen]);
    checksum ^= encrypted;
    cipherHex += toHexNibble(encrypted >> 4);
    cipherHex += toHexNibble(encrypted);
  }

  String packet = "SEC:";
  packet += cipherHex;
  packet += ":";
  packet += toHexNibble(checksum >> 4);
  packet += toHexNibble(checksum);
  packet += "\n";
  return packet;
}

static bool decodeSecurePacket(const String& packet, String& plainText) {
  if (!packet.startsWith("SEC:")) return false;

  int checksumSeparator = packet.lastIndexOf(':');
  if (checksumSeparator <= 4 || checksumSeparator + 2 >= (int)packet.length()) return false;

  String cipherHex = packet.substring(4, checksumSeparator);
  if ((cipherHex.length() % 2) != 0) return false;

  int hi = fromHexNibble(packet[checksumSeparator + 1]);
  int lo = fromHexNibble(packet[checksumSeparator + 2]);
  if (hi < 0 || lo < 0) return false;

  uint8_t expectedChecksum = (uint8_t)((hi << 4) | lo);
  uint8_t actualChecksum = 0;
  size_t keyLen = strlen(UART_KEY);

  plainText = "";
  plainText.reserve(cipherHex.length() / 2);

  for (size_t i = 0; i < cipherHex.length(); i += 2) {
    int byteHi = fromHexNibble(cipherHex[i]);
    int byteLo = fromHexNibble(cipherHex[i + 1]);
    if (byteHi < 0 || byteLo < 0) return false;

    uint8_t encrypted = (uint8_t)((byteHi << 4) | byteLo);
    actualChecksum ^= encrypted;
    plainText += (char)(encrypted ^ ((uint8_t)UART_KEY[(i / 2) % keyLen]));
  }

  return actualChecksum == expectedChecksum;
}

void taskUARTComm(void* pvParameters) {
  uint32_t lastTx = 0;
  // txCount variable removed since we no longer print it
  Serial1.setTimeout(50);
  Serial.println("[UART] Secure UART task started in silent mode.");

  for (;;) {
    // --- RX Logic: Check for incoming secure data from MCXC444 ---
    while (Serial1.available() > 0) {
      String rxStr = Serial1.readStringUntil('\n');
      rxStr.trim();

      if (rxStr.length() > 0) {
        String plainRx;
        if (!decodeSecurePacket(rxStr, plainRx)) {
          // Keep error logs for debugging
          Serial.printf("[UART RX ERROR] Invalid secure packet: %s\n", rxStr.c_str());
          continue;
        }

        // Successfully decrypted prints removed to save CPU

        if (plainRx.startsWith("MCX:")) {
          plainRx.remove(0, 4);
          plainRx.trim();
        }

        int tempVal = plainRx.toInt();
        
        // Accept only valid speed-band values from MCX feedback.
        if (tempVal >= 0 && tempVal <= 9) {
          g_mcxcValue = tempVal;
          g_newMcxcData = true; // Trigger Telegram update
          g_uploadDbFlag = true; // Trigger Database update
          // Success print removed
        } else {
          // Keep error logs for out-of-bounds data
          Serial.printf("[UART RX ERROR] Invalid Speedband: %s\n", plainRx.c_str());
        }
      }
    }

    // --- TX Logic: Broadcast secure packet to MCXC444 every 5 seconds ---
    if (millis() - lastTx >= 5000) {
      int localSpeeds[3] = {0, 0, 0};
      int rfidStatus = g_rfidUartFlag;

      // Safely access shared traffic data from LTA task
      if (xSemaphoreTake(g_trafficMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (int i = 0; i < 3; i++) localSpeeds[i] = g_speedBands[i];
        xSemaphoreGive(g_trafficMutex);
      }

      String plainTx = String(localSpeeds[0]) + "," +
                       String(localSpeeds[1]) + "," +
                       String(localSpeeds[2]) + "," +
                       String(rfidStatus);
      String securePacket = makeSecurePacket(plainTx);
      Serial1.print(securePacket);

      // Reset RFID flag after successful transmission
      if (rfidStatus == 1) g_rfidUartFlag = 0;

      // TX Success prints removed to save CPU

      lastTx = millis();
    }

    // Task heartbeat: check RX buffer every 50ms
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}