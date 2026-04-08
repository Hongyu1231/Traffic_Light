#ifndef TRAFFIC_GPIO_H_
#define TRAFFIC_GPIO_H_

#include "app_context.h"

void initGPIO(void);
void ledOn(TLED led);
void ledOff(TLED led);
void allLEDsOff(void);
void onPedestrianBuzzer(void);
void offPedestrianBuzzer(void);
void togglePedestrianLight(void);
void setPedestrianLightToRed(void);
void blinkPedestrianLight(void);

#endif /* TRAFFIC_GPIO_H_ */
