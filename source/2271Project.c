/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    2271Projecy.c
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

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    PRINTF("Intelligent Traffic System - MCXC444 Booting\r\n");

    sema = xSemaphoreCreateBinary();
    queue = xQueueCreate(QLEN, sizeof(TMessage));

    initGPIO();
    initInterrupt();
    initUART2(BAUD_RATE);

    xTaskCreate(parseUARTTask, "parseUART", configMINIMAL_STACK_SIZE + 100U, NULL, 2U, NULL);
    xTaskCreate(toggleVehicleLight, "toggleVehicleLight", configMINIMAL_STACK_SIZE + 100U, NULL, 1U, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
