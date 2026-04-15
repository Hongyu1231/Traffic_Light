#ifndef TRAFFIC_LDR_H_
#define TRAFFIC_LDR_H_

#include <stdint.h>

void initPhotoresistor(void);
uint16_t readPhotoresistorRaw(void);
uint16_t readPhotoresistorAverage(void);
uint32_t mapPhotoToYellowDelay(uint16_t raw);

#endif /* TRAFFIC_LDR_H_ */
