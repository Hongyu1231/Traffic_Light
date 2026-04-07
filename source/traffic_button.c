#include "traffic_button.h"

#include "board.h"
#include "app_context.h"

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

void PORTA_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    NVIC_ClearPendingIRQ(PORTA_IRQn);

    if ((PORTA->ISFR & (1UL << SWITCH_PIN)) != 0U) {
        PORTA->ISFR |= (1UL << SWITCH_PIN);
        xSemaphoreGiveFromISR(sema, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
