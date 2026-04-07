#include "traffic_uart.h"

#include <stdio.h>
#include <string.h>

#include "app_context.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

static char send_buffer[MAX_MSG_LEN];

void initUART2(uint32_t baud_rate)
{
    uint32_t bus_clk;
    uint32_t sbr;

    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    UART2->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    PORTE->PCR[UART_TX_PTE22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_TX_PTE22] |= PORT_PCR_MUX(4);

    PORTE->PCR[UART_RX_PTE23] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_RX_PTE23] |= PORT_PCR_MUX(4);

    bus_clk = CLOCK_GetBusClkFreq();
    sbr = (bus_clk + (baud_rate * 8U)) / (baud_rate * 16U);

    UART2->BDH &= ~UART_BDH_SBR_MASK;
    UART2->BDH |= (uint8_t)((sbr >> 8U) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFFU);

    UART2->C1 &= ~(UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK | UART_C1_PE_MASK | UART_C1_M_MASK);

    UART2->C2 |= UART_C2_RIE_MASK;
    UART2->C2 |= UART_C2_RE_MASK;

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);
}

void UART2_FLEXIO_IRQHandler(void)
{
    static int recv_ptr = 0;
    static int send_ptr = 0;
    static char recv_buffer[MAX_MSG_LEN];
    BaseType_t hpw = pdFALSE;

    if ((UART2->S1 & UART_S1_TDRE_MASK) != 0U) {
        if (send_buffer[send_ptr] == '\0') {
            send_ptr = 0;
            UART2->C2 &= ~UART_C2_TIE_MASK;
            UART2->C2 &= ~UART_C2_TE_MASK;
        } else {
            UART2->D = send_buffer[send_ptr++];
        }
    }

    if ((UART2->S1 & UART_S1_RDRF_MASK) != 0U) {
        char rx_data = (char)UART2->D;
        recv_buffer[recv_ptr++] = rx_data;

        if ((rx_data == '\n') || (recv_ptr >= ((int)MAX_MSG_LEN - 1))) {
            TMessage msg;

            recv_buffer[recv_ptr] = '\0';
            strncpy(msg.message, recv_buffer, MAX_MSG_LEN);
            msg.message[MAX_MSG_LEN - 1U] = '\0';
            xQueueSendFromISR(queue, &msg, &hpw);
            portYIELD_FROM_ISR(hpw);
            recv_ptr = 0;
        }
    }
}

void parseUARTTask(void *p)
{
    while (1) {
        TMessage msg;

        if (xQueueReceive(queue, &msg, portMAX_DELAY) == pdTRUE) {
            int s1 = 0;
            int s2 = 0;
            int s3 = 0;

            if (sscanf(msg.message, "SPEEDS:%d,%d,%d", &s1, &s2, &s3) == 3) {
                current_speed_bands[0] = s1;
                current_speed_bands[1] = s2;
                current_speed_bands[2] = s3;

                PRINTF("Updated Traffic Speeds -> R1: %d, R2: %d, R3: %d\r\n",
                       current_speed_bands[0],
                       current_speed_bands[1],
                       current_speed_bands[2]);
            }
        }
    }
}
