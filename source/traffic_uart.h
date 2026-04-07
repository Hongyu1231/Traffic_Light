#ifndef TRAFFIC_UART_H_
#define TRAFFIC_UART_H_

#include <stdint.h>

void initUART2(uint32_t baud_rate);
void parseUARTTask(void *p);

#endif /* TRAFFIC_UART_H_ */
