#include "traffic_ldr.h"

#include "board.h"

#define PHOTO_ADC_CHANNEL      14U
#define PHOTO_ADC_MAX          4095U

#define YELLOW_TIME_MIN_MS     3000U
#define YELLOW_TIME_MAX_MS     7000U

void initPhotoresistor(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    PORTC->PCR[0] = PORT_PCR_MUX(0);

    ADC0->CFG1 = ADC_CFG1_ADICLK(0) |
                 ADC_CFG1_MODE(1)   |
                 ADC_CFG1_ADLSMP(1) |
                 ADC_CFG1_ADIV(1);

    ADC0->CFG2 = 0U;

    ADC0->SC2 = ADC_SC2_ADTRG(0) |
                ADC_SC2_REFSEL(0);

    ADC0->SC3 = ADC_SC3_AVGE(1) |
                ADC_SC3_AVGS(3);
}

uint16_t readPhotoresistorRaw(void)
{
    ADC0->SC1[0] = ADC_SC1_ADCH(PHOTO_ADC_CHANNEL);

    while ((ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0U) {
    }

    return (uint16_t)ADC0->R[0];
}

uint16_t readPhotoresistorAverage(void)
{
    uint32_t sum = 0U;
    int i;

    for (i = 0; i < 8; i++) {
        sum += readPhotoresistorRaw();
    }

    return (uint16_t)(sum / 8U);
}

uint32_t mapPhotoToYellowDelay(uint16_t raw)
{
    uint32_t darkness;
    uint32_t range;

    /* With 100k to 3.3V and the LDR to GND:
       brighter light -> lower ADC reading
       darker light   -> higher ADC reading
       so raw already represents darkness. */
    darkness = raw;

    if (darkness > PHOTO_ADC_MAX) {
        darkness = PHOTO_ADC_MAX;
    }

    range = YELLOW_TIME_MAX_MS - YELLOW_TIME_MIN_MS;

    return YELLOW_TIME_MIN_MS + ((darkness * range) / PHOTO_ADC_MAX);
}
