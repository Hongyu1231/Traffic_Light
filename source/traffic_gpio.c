#include "traffic_gpio.h"

#include "board.h"
#include "fsl_common.h"
#include "FreeRTOS.h"
#include "task.h"

void initGPIO(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK;

    PORTD->PCR[GREEN_PIN] = PORT_PCR_MUX(1);
    GPIOD->PDDR |= (1UL << GREEN_PIN);

    PORTE->PCR[RED_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[BLUE_PIN] = PORT_PCR_MUX(1);
    GPIOE->PDDR |= (1UL << RED_PIN) | (1UL << BLUE_PIN);

    PORTC->PCR[PEDESTRIAN_BUZZER] = PORT_PCR_MUX(1);
    GPIOC->PDDR |= (1UL << PEDESTRIAN_BUZZER);

    PORTA->PCR[PEDESTRIAN_RED] = PORT_PCR_MUX(1);
    GPIOA->PDDR |= (1UL << PEDESTRIAN_RED);

    PORTA->PCR[PEDESTRIAN_GREEN] = PORT_PCR_MUX(1);
    GPIOA->PDDR |= (1UL << PEDESTRIAN_GREEN);

    allLEDsOff();
    setPedestrianLightToRed();
    offPedestrianBuzzer();
}

void ledOn(TLED led)
{
    switch (led) {
    case RED:
        GPIOE->PCOR = (1UL << RED_PIN);
        break;
    case GREEN:
        GPIOD->PCOR = (1UL << GREEN_PIN);
        break;
    case BLUE:
        GPIOE->PCOR = (1UL << BLUE_PIN);
        break;
    default:
        break;
    }
}

void ledOff(TLED led)
{
    switch (led) {
    case RED:
        GPIOE->PSOR = (1UL << RED_PIN);
        break;
    case GREEN:
        GPIOD->PSOR = (1UL << GREEN_PIN);
        break;
    case BLUE:
        GPIOE->PSOR = (1UL << BLUE_PIN);
        break;
    default:
        break;
    }
}

void allLEDsOff(void)
{
    ledOff(RED);
    ledOff(GREEN);
    ledOff(BLUE);
}

void onPedestrianBuzzer(void)
{
    GPIOC->PSOR = (1UL << PEDESTRIAN_BUZZER);
}

void offPedestrianBuzzer(void)
{
    GPIOC->PCOR = (1UL << PEDESTRIAN_BUZZER);
}

void togglePedestrianLight(void)
{
    GPIOA->PCOR = (1UL << PEDESTRIAN_RED);
    GPIOA->PSOR = (1UL << PEDESTRIAN_GREEN);
}

void setPedestrianLightToRed(void)
{
    GPIOA->PSOR = (1UL << PEDESTRIAN_RED);
    GPIOA->PCOR = (1UL << PEDESTRIAN_GREEN);
}

void blinkPedestrianLight(void)
{
    int i;

    for (i = 0; i < 5; i++) {
        GPIOA->PCOR = (1UL << PEDESTRIAN_GREEN);
        offPedestrianBuzzer();
        vTaskDelay(pdMS_TO_TICKS(500U));

        GPIOA->PSOR = (1UL << PEDESTRIAN_GREEN);
        onPedestrianBuzzer();
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}
