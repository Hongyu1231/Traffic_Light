#include "app_context.h"

SemaphoreHandle_t sema = NULL;
QueueHandle_t queue = NULL;
volatile int current_speed_bands[3] = {0, 0, 0};
volatile int lcd_countdown_seconds = 0;
volatile TRfidState current_rfid_state = RFID_STATE_NONE;
volatile bool pedestrian_phase_active = false;
volatile bool pedestrian_button_latched = false;
