#include <WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = "Hongyu"; 
const char* password = "12345678";
String LTASpeedBandApiKey = "RqgAcv3YQW+ZcgFjTmmGHQ==";

// Global Resources
SemaphoreHandle_t g_trafficMutex;
SemaphoreHandle_t g_wifiMutex; 
int g_speedBands[3] = {0, 0, 0}; 

// RFID & MCXC Data
volatile int g_rfidUartFlag = 0; 
volatile bool g_uploadDbFlag = false;
volatile int g_mcxcValue = 0;
volatile bool g_newMcxcData = false;

void taskLTA_Pipeline(void* pvParameters);
void taskPollRFID(void* pvParameters);
void taskUARTComm(void* pvParameters);
void taskTelegramAlert(void* pvParameters);
void taskDatabaseUpload(void* pvParameters);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- System Booting: MCXC Feedback Mode ---");

  Serial1.begin(9600, SERIAL_8N1, 18, 17);
  Serial.println("[UART] Serial1 started on TX=17 RX=18 @ 9600 8N1");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");

  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  g_trafficMutex = xSemaphoreCreateMutex();
  g_wifiMutex = xSemaphoreCreateMutex();

  xTaskCreate(taskLTA_Pipeline, "LTA_Task", 10240, NULL, 1, NULL);
  xTaskCreate(taskUARTComm, "UART_Task", 4096, NULL, 3, NULL);
  xTaskCreate(taskPollRFID, "RFID_Task", 4096, NULL, 2, NULL);
  xTaskCreate(taskTelegramAlert, "Tele_Task", 10240, NULL, 1, NULL);
  xTaskCreate(taskDatabaseUpload, "Task_DB", 4096, NULL, 1, NULL);

  Serial.println("System is ready.");
}

void loop() {}
