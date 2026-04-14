#include "app_context.h"

SemaphoreHandle_t sema = NULL;
QueueHandle_t queue = NULL;
volatile int current_speed_bands[3] = {0, 0, 0};
volatile int authorized_rfid_request = 0;
