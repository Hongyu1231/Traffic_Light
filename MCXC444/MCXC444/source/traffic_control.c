#include "traffic_control.h"

#include <stdint.h>

#include "app_context.h"
#include "fsl_debug_console.h"
#include "traffic_gpio.h"
#include "traffic_ldr.h"
#include "traffic_lcd2004.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

void toggleVehicleLight(void *p)
{
    (void)p;

    for (;;) {
        int current_band;
        uint32_t time_window;
        uint32_t step_size;
        uint32_t dynamic_green_delay;
        uint16_t ldr_raw;
        uint32_t yellow_delay_ms;
        uint32_t pedestrian_green_ms;
        TRfidState preview_rfid_state;
        TRfidState latched_rfid_state;

        allLEDsOff();
        ledOn(GREEN);
        setPedestrianLightToRed();
        offPedestrianBuzzer();

        if (xSemaphoreTake(sema, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        current_band = current_speed_bands[0];

        if (current_band > S_MAX_BAND) {
            current_band = S_MAX_BAND;
        }
        if (current_band < S_MIN_BAND) {
            current_band = S_MIN_BAND;
        }

        time_window = T_MAX_MS - T_MIN_MS;
        step_size = (uint32_t)(S_MAX_BAND - S_MIN_BAND);
        dynamic_green_delay = T_MAX_MS - ((((uint32_t)(current_band - S_MIN_BAND)) * time_window) / step_size);
        preview_rfid_state = current_rfid_state;

        PRINTF("Pedestrian Waiting! Live Band: %d, Wait Delay: %d ms, Fixed Green: %d ms\r\n",
               current_band,
               (int)dynamic_green_delay,
               (int)((preview_rfid_state == RFID_STATE_VALID) ? PEDESTRIAN_GREEN_EXTENDED_MS : PEDESTRIAN_GREEN_MS));

        vTaskDelay(pdMS_TO_TICKS(dynamic_green_delay));

        ldr_raw = readPhotoresistorAverage();
        yellow_delay_ms = mapPhotoToYellowDelay(ldr_raw);

        PRINTF("Photoresistor raw=%u, Yellow delay=%u ms\r\n",
               ldr_raw,
               (unsigned int)yellow_delay_ms);

        ledOn(RED);
        vTaskDelay(pdMS_TO_TICKS(yellow_delay_ms));

        taskENTER_CRITICAL();
        pedestrian_phase_active = true;
        latched_rfid_state = current_rfid_state;
        taskEXIT_CRITICAL();

        ledOff(GREEN);
        togglePedestrianLight();
        pedestrian_green_ms = (latched_rfid_state == RFID_STATE_VALID) ? PEDESTRIAN_GREEN_EXTENDED_MS : PEDESTRIAN_GREEN_MS;
        blinkPedestrianLight(pedestrian_green_ms / 1000U);

        taskENTER_CRITICAL();
        current_rfid_state = RFID_STATE_NONE;
        pedestrian_phase_active = false;
        pedestrian_button_latched = false;
        taskEXIT_CRITICAL();
        setPedestrianLightToRed();
        lcd2004SetStatus(LCD2004_STATUS_IDLE);
    }
}
