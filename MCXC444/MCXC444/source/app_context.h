/*
 * Shared traffic-light application definitions.
 */
#ifndef APP_CONTEXT_H_
#define APP_CONTEXT_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define RED_PIN             31U
#define GREEN_PIN           5U
#define BLUE_PIN            29U

#define SWITCH_PIN          4U
#define HALL_SENSOR_1_PIN   12U
#define HALL_SENSOR_2_PIN   13U
#define PEDESTRIAN_BUZZER   4U

#define PEDESTRIAN_RED      1U //PTC1
#define PEDESTRIAN_GREEN    2U //PTC2

#define UART_TX_PTE22       22U
#define UART_RX_PTE23       23U
#define BAUD_RATE           9600U
#define UART2_INT_PRIO      128U

#define MAX_MSG_LEN         256U
#define QLEN                5U

#define T_MAX_MS            15000U
#define T_MIN_MS            5000U
#define PEDESTRIAN_GREEN_MS 10000U
#define EXTENDED_PEDESTRIAN_GREEN_MS 15000U
#define S_MAX_BAND          8
#define S_MIN_BAND          1

#define HALL_SENSOR_1_ACTIVE_HIGH 1U
#define HALL_SENSOR_2_ACTIVE_HIGH 0U

#define SPEED_DISTANCE_CM        15U
#define SPEED_MIN_VALID_CM_PER_S 1U
#define SPEED_MAX_CM_PER_S       18U
#define SPEED_HARD_MAX_CM_PER_S  40U
#define SPEED_BAND_STEP_CM_PER_S 2U
#define SPEED_SAMPLE_MS          10U
#define SPEED_CLEAR_DELAY_MS     100U

typedef enum tl {
    RED,
    GREEN,
    BLUE
} TLED;

typedef struct tm {
    char message[MAX_MSG_LEN];
} TMessage;

extern SemaphoreHandle_t sema;
extern QueueHandle_t queue;
extern volatile int current_speed_bands[3];
extern volatile int authorized_rfid_request;

#endif /* APP_CONTEXT_H_ */
