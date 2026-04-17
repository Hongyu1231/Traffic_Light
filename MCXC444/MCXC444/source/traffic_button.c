#include "traffic_button.h"

#include <stdbool.h>

#include "board.h"
#include "app_context.h"
#include "fsl_debug_console.h"
#include "traffic_uart.h"

typedef enum speed_trap_state {
    SPEED_TRAP_IDLE = 0,
    SPEED_TRAP_ARMED,
    SPEED_TRAP_WAIT_CLEAR
} TSpeedTrapState;

static bool hallSensorActive(uint32_t pin, uint32_t active_high)
{
    uint32_t pin_state = (GPIOA->PDIR & (1UL << pin)) ? 1UL : 0UL;
    return active_high ? (pin_state != 0UL) : (pin_state == 0UL);
}

static uint8_t convertSpeedToBand(uint32_t speed_tenths_cm_per_s)
{
    if (speed_tenths_cm_per_s < SPEED_MIN_VALID_TENTHS_CM_PER_S) {
        return 0U;
    }

    if (speed_tenths_cm_per_s > SPEED_HARD_MAX_TENTHS_CM_PER_S) {
        return 0U;
    }

    if (speed_tenths_cm_per_s >= SPEED_MAX_TENTHS_CM_PER_S) {
        return 9U;
    }

    return (uint8_t)(speed_tenths_cm_per_s / SPEED_BAND_STEP_TENTHS_CM_PER_S);
}

void initHallSensorPins(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    PORTA->PCR[HALL_SENSOR_1_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTA->PCR[HALL_SENSOR_1_PIN] |= PORT_PCR_MUX(1);
    PORTA->PCR[HALL_SENSOR_1_PIN] &= ~(PORT_PCR_PS_MASK | PORT_PCR_PE_MASK);
    PORTA->PCR[HALL_SENSOR_1_PIN] |= PORT_PCR_PS(1) | PORT_PCR_PE(1);

    PORTA->PCR[HALL_SENSOR_2_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTA->PCR[HALL_SENSOR_2_PIN] |= PORT_PCR_MUX(1);
    PORTA->PCR[HALL_SENSOR_2_PIN] &= ~(PORT_PCR_PS_MASK | PORT_PCR_PE_MASK);
    PORTA->PCR[HALL_SENSOR_2_PIN] |= PORT_PCR_PS(1) | PORT_PCR_PE(1);

    GPIOA->PDDR &= ~((1UL << HALL_SENSOR_1_PIN) | (1UL << HALL_SENSOR_2_PIN));
}

void initInterrupt(void)
{
    NVIC_DisableIRQ(PORTA_IRQn);

    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_MUX(1);

    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_PS_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_PS(1);
    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_PE_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_PE(1);

    GPIOA->PDDR &= ~(1UL << SWITCH_PIN);

    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_IRQC_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_IRQC(0xAU);

    NVIC_SetPriority(PORTA_IRQn, 192U);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
}

void speedTrapTask(void *p)
{
    TSpeedTrapState state = SPEED_TRAP_IDLE;
    TickType_t start_tick = 0U;
    bool sensor1_prev;
    bool sensor2_prev;

    (void)p;

    sensor1_prev = hallSensorActive(HALL_SENSOR_1_PIN, HALL_SENSOR_1_ACTIVE_HIGH);
    sensor2_prev = hallSensorActive(HALL_SENSOR_2_PIN, HALL_SENSOR_2_ACTIVE_HIGH);

    PRINTF("Hall speed trap ready: S1=PTA12 S2=PTA13 distance=%u cm\r\n",
           (unsigned int)SPEED_DISTANCE_CM);

    for (;;) {
        bool sensor1_active = hallSensorActive(HALL_SENSOR_1_PIN, HALL_SENSOR_1_ACTIVE_HIGH);
        bool sensor2_active = hallSensorActive(HALL_SENSOR_2_PIN, HALL_SENSOR_2_ACTIVE_HIGH);
        bool sensor1_edge = sensor1_active && !sensor1_prev;
        bool sensor2_edge = sensor2_active && !sensor2_prev;

        if ((state == SPEED_TRAP_IDLE) && sensor1_edge) {
            start_tick = xTaskGetTickCount();
            state = SPEED_TRAP_ARMED;
            PRINTF("Sensor 1 detected on PTA12, timer started at tick=%lu\r\n",
                   (unsigned long)start_tick);
        }

        if ((state == SPEED_TRAP_ARMED) && sensor2_edge) {
            TickType_t end_tick = xTaskGetTickCount();
            uint32_t elapsed_ms = (uint32_t)pdTICKS_TO_MS(end_tick - start_tick);

            state = SPEED_TRAP_WAIT_CLEAR;
            PRINTF("Sensor 2 detected on PTA13, timer stopped\r\n");

            if (elapsed_ms > 0U) {
                uint32_t speed_tenths_cm_per_s =
                    (uint32_t)(((SPEED_DISTANCE_CM * 10000UL) + (elapsed_ms / 2UL)) / elapsed_ms);
                uint8_t speed_band = convertSpeedToBand(speed_tenths_cm_per_s);

                PRINTF("Speed detected: elapsed=%lu ms speed=%lu.%lu cm/s band=%u\r\n",
                       (unsigned long)elapsed_ms,
                       (unsigned long)(speed_tenths_cm_per_s / 10UL),
                       (unsigned long)(speed_tenths_cm_per_s % 10UL),
                       (unsigned int)speed_band);

                if (speed_band > 0U) {
                    sendSpeedBand(speed_band);
                } else {
                    PRINTF("Speed reading rejected as invalid/noise\r\n");
                }
            }
        }

        if (state == SPEED_TRAP_WAIT_CLEAR) {
            if (!sensor1_active && !sensor2_active) {
                vTaskDelay(pdMS_TO_TICKS(SPEED_CLEAR_DELAY_MS));

                sensor1_active = hallSensorActive(HALL_SENSOR_1_PIN, HALL_SENSOR_1_ACTIVE_HIGH);
                sensor2_active = hallSensorActive(HALL_SENSOR_2_PIN, HALL_SENSOR_2_ACTIVE_HIGH);

                if (!sensor1_active && !sensor2_active) {
                    state = SPEED_TRAP_IDLE;
                    PRINTF("Sensors clear, hall speed trap re-armed\r\n");
                }
            }
        }

        sensor1_prev = sensor1_active;
        sensor2_prev = sensor2_active;
        vTaskDelay(pdMS_TO_TICKS(SPEED_SAMPLE_MS));
    }
}

void PORTA_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    NVIC_ClearPendingIRQ(PORTA_IRQn);

    if ((PORTA->ISFR & (1UL << SWITCH_PIN)) != 0U) {
        PORTA->ISFR |= (1UL << SWITCH_PIN);
        if (!pedestrian_button_latched) {
            pedestrian_button_latched = true;
            xSemaphoreGiveFromISR(sema, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}
