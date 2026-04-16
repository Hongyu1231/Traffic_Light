#ifndef TRAFFIC_LCD2004_H_
#define TRAFFIC_LCD2004_H_

typedef enum lcd2004_status
{
    LCD2004_STATUS_IDLE = 0,
    LCD2004_STATUS_INVALID,
    LCD2004_STATUS_PROCEED
} TLcd2004Status;

void initLCD2004(void);
void lcd2004SetStatus(TLcd2004Status status);
void lcd2004ResetToIdle(void);
void lcd2004Task(void *p);

#endif /* TRAFFIC_LCD2004_H_ */
