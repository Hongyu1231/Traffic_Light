#ifndef TRAFFIC_BUTTON_H_
#define TRAFFIC_BUTTON_H_

#include "FreeRTOS.h"
#include "task.h"

void initInterrupt(void);
void initHallSensorPins(void);
void speedTrapTask(void *p);

#endif /* TRAFFIC_BUTTON_H_ */
