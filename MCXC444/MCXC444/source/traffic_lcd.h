#ifndef TRAFFIC_LCD_H_
#define TRAFFIC_LCD_H_

#include <stdint.h>

void initSLCD(void);
void lcdTask(void *p);
void setLCDCountdownValue(int value);

#endif /* TRAFFIC_LCD_H_ */
