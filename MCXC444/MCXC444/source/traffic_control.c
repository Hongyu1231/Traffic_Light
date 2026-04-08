#include "traffic_control.h"

#include <stdint.h>

#include "app_context.h"
#include "fsl_debug_console.h"
#include "traffic_gpio.h"

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

        PRINTF("Pedestrian Waiting! Live Band: %d, Green Extension: %d ms\r\n",
               current_band,
               (int)dynamic_green_delay);

        vTaskDelay(pdMS_TO_TICKS(dynamic_green_delay));

        ledOn(RED);
        vTaskDelay(pdMS_TO_TICKS(3000U));

        ledOff(GREEN);
        togglePedestrianLight();
        vTaskDelay(pdMS_TO_TICKS(10000U));

        blinkPedestrianLight();
    }
}
