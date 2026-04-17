/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    2271Project.c
 * @brief   Application entry point.
 */
#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "peripherals.h"
#include "pin_mux.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "app_context.h"
#include "traffic_button.h"
#include "traffic_control.h"
#include "traffic_uart.h"
#include "traffic_gpio.h"
#include "traffic_ldr.h"
#include "traffic_lcd.h"
#include "traffic_lcd2004.h"

int main(void)
{
    BaseType_t uartTaskOk;
    BaseType_t lightTaskOk;
    BaseType_t speedTrapTaskOk;
    BaseType_t lcdTaskOk;
    BaseType_t lcd2004TaskOk;

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("Intelligent Traffic System - MCXC444 Booting\r\n");

    sema = xSemaphoreCreateBinary();
    stateMutex = xSemaphoreCreateMutex();
    queue = xQueueCreate(QLEN, sizeof(TMessage));


    initGPIO();
    initInterrupt();
    initHallSensorPins();
    initUART2(BAUD_RATE);
    initPhotoresistor();
    initSLCD();
    initLCD2004();

    uartTaskOk = xTaskCreate(parseUARTTask, "parseUART", configMINIMAL_STACK_SIZE + 768U, NULL, 2U, NULL);
    lightTaskOk = xTaskCreate(toggleVehicleLight, "toggleVehicleLight", configMINIMAL_STACK_SIZE + 512U, NULL, 1U, NULL);
    speedTrapTaskOk = xTaskCreate(speedTrapTask, "speedTrap", configMINIMAL_STACK_SIZE + 512U, NULL, 2U, NULL);
    lcdTaskOk = xTaskCreate(lcdTask, "LCD_task", configMINIMAL_STACK_SIZE + 256U, NULL, 1U, NULL);
    lcd2004TaskOk = xTaskCreate(lcd2004Task, "LCD2004_task", configMINIMAL_STACK_SIZE + 384U, NULL, 1U, NULL);

    PRINTF("Task create -> parseUART=%ld toggleVehicleLight=%ld speedTrap=%ld LCD_task=%ld LCD2004_task=%ld\r\n",
           (long)uartTaskOk,
           (long)lightTaskOk,
           (long)speedTrapTaskOk,
            (long)lcdTaskOk,
           (long)lcd2004TaskOk);

    vTaskStartScheduler();

    while (1) {
    }
}
