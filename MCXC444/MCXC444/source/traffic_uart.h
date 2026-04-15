#ifndef TRAFFIC_UART_H_
#define TRAFFIC_UART_H_

#include <stdint.h>

void initUART2(uint32_t baud_rate);
void parseUARTTask(void *p);
void sendMessage(const char *message);
void sendSpeedBand(uint8_t band);

#endif /* TRAFFIC_UART_H_ */
