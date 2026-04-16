#include "traffic_servo.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_context.h"
#include "board.h"
#include "fsl_clock.h"
#include "fsl_debug_console.h"
#include "traffic_lcd2004.h"

#include "FreeRTOS.h"
#include "task.h"

#define SERVO_PORT       PORTE
#define SERVO_GPIO       GPIOE
#define SERVO_PIN        SERVO_PIN_PTE20
#define SERVO_PERIOD_US  20000U
#define SERVO_MIN_US     500U
#define SERVO_MAX_US     2500U
#define SERVO_STEP_US    50U
#define SERVO_HOLD_MS    200U

static volatile bool servo_is_open = false;
static volatile TickType_t servo_close_deadline = 0U;
static bool servo_reset_pending = false;

typedef struct servo_sweep_state
{
    uint32_t pulse_us;
    bool sweeping_up;
    uint32_t hold_cycles_remaining;
} servo_sweep_state_t;

static servo_sweep_state_t servo_state = {
    SERVO_MIN_US,
    true,
    SERVO_HOLD_MS / 20U
};

static void delayUs(uint32_t us)
{
    SDK_DelayAtLeastUs(us, CLOCK_GetCoreSysClkFreq());
}

static void servoWritePulse(uint32_t pulseWidthUs)
{
    if (pulseWidthUs < SERVO_MIN_US) {
        pulseWidthUs = SERVO_MIN_US;
    }
    if (pulseWidthUs > SERVO_MAX_US) {
        pulseWidthUs = SERVO_MAX_US;
    }

    SERVO_GPIO->PSOR = (1UL << SERVO_PIN);
    delayUs(pulseWidthUs);

    SERVO_GPIO->PCOR = (1UL << SERVO_PIN);
}

static void servoResetSweep(void)
{
    servo_state.pulse_us = SERVO_MIN_US;
    servo_state.sweeping_up = true;
    servo_state.hold_cycles_remaining = SERVO_HOLD_MS / 20U;
}

static void servoAdvanceSweep(void)
{
    if (servo_state.hold_cycles_remaining > 1U) {
        servo_state.hold_cycles_remaining--;
        return;
    }

    servo_state.hold_cycles_remaining = SERVO_HOLD_MS / 20U;

    if (servo_state.sweeping_up) {
        if ((servo_state.pulse_us + SERVO_STEP_US) >= SERVO_MAX_US) {
            servo_state.pulse_us = SERVO_MAX_US;
            servo_state.sweeping_up = false;
        } else {
            servo_state.pulse_us += SERVO_STEP_US;
        }
    } else {
        if (servo_state.pulse_us <= (SERVO_MIN_US + SERVO_STEP_US)) {
            servo_state.pulse_us = SERVO_MIN_US;
            servo_state.sweeping_up = true;
        } else {
            servo_state.pulse_us -= SERVO_STEP_US;
        }
    }
}

void initServo(void)
{
    CLOCK_EnableClock(kCLOCK_PortE);

    SERVO_PORT->PCR[SERVO_PIN] = PORT_PCR_MUX(1);
    SERVO_GPIO->PDDR |= (1UL << SERVO_PIN);
    SERVO_GPIO->PCOR = (1UL << SERVO_PIN);
    servoResetSweep();

    PRINTF("Servo ready on PTE20 GPIO sweep %u-%u us step %u us\r\n",
           (unsigned int)SERVO_MIN_US,
           (unsigned int)SERVO_MAX_US,
           (unsigned int)SERVO_STEP_US);
}

void servoRequestOpen(void)
{
    taskENTER_CRITICAL();
    servo_is_open = true;
    servo_close_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(SERVO_OPEN_HOLD_MS);
    servo_reset_pending = false;
    servoResetSweep();
    taskEXIT_CRITICAL();

    PRINTF("Servo sweep started for %u ms\r\n", (unsigned int)SERVO_OPEN_HOLD_MS);
}

void servoTask(void *p)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    (void)p;

    for (;;) {
        bool should_sweep = false;
        bool should_reset_lcd = false;

        taskENTER_CRITICAL();
        if (servo_is_open) {
            if ((int32_t)(xTaskGetTickCount() - servo_close_deadline) >= 0) {
                servo_is_open = false;
                servo_reset_pending = true;
            } else {
                should_sweep = true;
            }
        }
        if (servo_reset_pending) {
            servo_reset_pending = false;
            should_reset_lcd = true;
        }
        taskEXIT_CRITICAL();

        if (should_sweep) {
            servoWritePulse(servo_state.pulse_us);
            servoAdvanceSweep();
            xTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20U));
        } else {
            SERVO_GPIO->PCOR = (1UL << SERVO_PIN);
            vTaskDelay(pdMS_TO_TICKS(20U));
            lastWakeTime = xTaskGetTickCount();
        }

        if (should_reset_lcd) {
            servoResetSweep();
            lcd2004ResetToIdle();
        }
    }
}
