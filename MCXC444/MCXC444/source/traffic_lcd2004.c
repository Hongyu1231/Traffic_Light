#include "traffic_lcd2004.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_context.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"

#include "FreeRTOS.h"
#include "task.h"

#define LCD2004_I2C                 I2C0
#define LCD2004_I2C_ADDR            0x27U
#define LCD2004_I2C_BAUD            100000U
#define LCD2004_COLUMNS             20U
#define LCD2004_STATUS_POLL_MS      50U

#define LCD2004_PIN_RS              0x01U
#define LCD2004_PIN_RW              0x02U
#define LCD2004_PIN_EN              0x04U
#define LCD2004_PIN_BACKLIGHT       0x08U

#define LCD2004_CMD_CLEAR           0x01U
#define LCD2004_CMD_HOME            0x02U
#define LCD2004_CMD_ENTRY_MODE      0x06U
#define LCD2004_CMD_DISPLAY_ON      0x0CU
#define LCD2004_CMD_FUNCTION_SET    0x28U
#define LCD2004_CMD_SET_DDRAM       0x80U
#define LCD2004_I2C_SCAN_START      0x20U
#define LCD2004_I2C_SCAN_END        0x3FU

static volatile TLcd2004Status current_status = LCD2004_STATUS_IDLE;
static volatile bool lcd2004_available = false;
static uint8_t lcd2004_active_addr = LCD2004_I2C_ADDR;

static void lcd2004DelayMs(uint32_t delay_ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        SDK_DelayAtLeastUs(delay_ms * 1000U, CLOCK_GetCoreSysClkFreq());
    } else {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static status_t lcd2004WriteByte(uint8_t value)
{
    i2c_master_transfer_t transfer;

    transfer.flags = kI2C_TransferDefaultFlag;
    transfer.slaveAddress = lcd2004_active_addr;
    transfer.direction = kI2C_Write;
    transfer.subaddress = 0U;
    transfer.subaddressSize = 0U;
    transfer.data = &value;
    transfer.dataSize = 1U;

    return I2C_MasterTransferBlocking(LCD2004_I2C, &transfer);
}

static status_t lcd2004ProbeAddress(uint8_t address)
{
    i2c_master_transfer_t transfer;
    uint8_t value = LCD2004_PIN_BACKLIGHT;

    transfer.flags = kI2C_TransferDefaultFlag;
    transfer.slaveAddress = address;
    transfer.direction = kI2C_Write;
    transfer.subaddress = 0U;
    transfer.subaddressSize = 0U;
    transfer.data = &value;
    transfer.dataSize = 1U;

    return I2C_MasterTransferBlocking(LCD2004_I2C, &transfer);
}

static bool lcd2004ScanBus(void)
{
    bool found = false;
    uint8_t address;

    PRINTF("Scanning I2C0 for LCD backpack...\r\n");

    for (address = LCD2004_I2C_SCAN_START; address <= LCD2004_I2C_SCAN_END; address++) {
        if (lcd2004ProbeAddress(address) == kStatus_Success) {
            PRINTF("I2C device found at 0x%02X\r\n", address);

            if (!found) {
                lcd2004_active_addr = address;
                found = true;
            }
        }
    }

    return found;
}

static void lcd2004WriteExpander(uint8_t value)
{
    (void)lcd2004WriteByte((uint8_t)(value | LCD2004_PIN_BACKLIGHT));
}

static void lcd2004PulseEnable(uint8_t value)
{
    lcd2004WriteExpander((uint8_t)(value | LCD2004_PIN_EN));
    lcd2004DelayMs(1U);
    lcd2004WriteExpander((uint8_t)(value & (uint8_t)(~LCD2004_PIN_EN)));
    lcd2004DelayMs(1U);
}

static void lcd2004Write4Bits(uint8_t nibble_and_flags)
{
    lcd2004WriteExpander(nibble_and_flags);
    lcd2004PulseEnable(nibble_and_flags);
}

static void lcd2004Send(uint8_t value, uint8_t mode)
{
    uint8_t high = (uint8_t)(value & 0xF0U);
    uint8_t low = (uint8_t)((value << 4U) & 0xF0U);

    lcd2004Write4Bits((uint8_t)(high | mode));
    lcd2004Write4Bits((uint8_t)(low | mode));
}

static void lcd2004Command(uint8_t command)
{
    lcd2004Send(command, 0U);
    lcd2004DelayMs(2U);
}

static void lcd2004Data(uint8_t data)
{
    lcd2004Send(data, LCD2004_PIN_RS);
}

static void lcd2004SetCursor(uint8_t row, uint8_t col)
{
    static const uint8_t row_offsets[4] = {0x00U, 0x40U, 0x14U, 0x54U};
    lcd2004Command((uint8_t)(LCD2004_CMD_SET_DDRAM | (row_offsets[row] + col)));
}

static void lcd2004WritePaddedLine(uint8_t row, const char *text)
{
    size_t len = strlen(text);
    size_t i;

    lcd2004SetCursor(row, 0U);

    for (i = 0U; i < LCD2004_COLUMNS; i++) {
        uint8_t ch = (i < len) ? (uint8_t)text[i] : (uint8_t)' ';
        lcd2004Data(ch);
    }
}

static void lcd2004RenderStatus(TLcd2004Status status)
{
    switch (status) {
    case LCD2004_STATUS_INVALID:
        lcd2004WritePaddedLine(0U, "Invalid User,");
        lcd2004WritePaddedLine(1U, "Try Again");
        lcd2004WritePaddedLine(2U, "");
        lcd2004WritePaddedLine(3U, "");
        break;
    case LCD2004_STATUS_PROCEED:
        lcd2004WritePaddedLine(0U, "Valid User,");
        lcd2004WritePaddedLine(1U, "Extension Granted");
        lcd2004WritePaddedLine(2U, "");
        lcd2004WritePaddedLine(3U, "");
        break;
    case LCD2004_STATUS_IDLE:
    default:
        lcd2004WritePaddedLine(0U, "Tap Card");
        lcd2004WritePaddedLine(1U, "");
        lcd2004WritePaddedLine(2U, "");
        lcd2004WritePaddedLine(3U, "");
        break;
    }
}

void initLCD2004(void)
{
    i2c_master_config_t config;

    CLOCK_EnableClock(kCLOCK_PortB);
    PORTB->PCR[LCD_I2C_SCL_PTB0] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[LCD_I2C_SCL_PTB0] |= PORT_PCR_MUX(2);
    PORTB->PCR[LCD_I2C_SDA_PTB1] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[LCD_I2C_SDA_PTB1] |= PORT_PCR_MUX(2);

    I2C_MasterGetDefaultConfig(&config);
    config.baudRate_Bps = LCD2004_I2C_BAUD;
    I2C_MasterInit(LCD2004_I2C, &config, CLOCK_GetFreq(I2C0_CLK_SRC));

    lcd2004_active_addr = LCD2004_I2C_ADDR;

    if (lcd2004ProbeAddress(lcd2004_active_addr) != kStatus_Success) {
        if (!lcd2004ScanBus()) {
            PRINTF("LCD2004 not detected on I2C0 (checked 0x%02X-0x%02X)\r\n",
                   LCD2004_I2C_SCAN_START,
                   LCD2004_I2C_SCAN_END);
            lcd2004_available = false;
            return;
        }

        PRINTF("LCD2004 using detected I2C address 0x%02X\r\n", lcd2004_active_addr);
    } else {
        PRINTF("LCD2004 responded at configured I2C address 0x%02X\r\n", lcd2004_active_addr);
    }

    if (lcd2004WriteByte(LCD2004_PIN_BACKLIGHT) != kStatus_Success) {
        PRINTF("LCD2004 detected but expander write failed at 0x%02X\r\n", lcd2004_active_addr);
        lcd2004_available = false;
        return;
    }

    lcd2004DelayMs(50U);
    lcd2004Write4Bits(0x30U);
    lcd2004DelayMs(5U);
    lcd2004Write4Bits(0x30U);
    lcd2004DelayMs(5U);
    lcd2004Write4Bits(0x30U);
    lcd2004DelayMs(2U);
    lcd2004Write4Bits(0x20U);

    lcd2004Command(LCD2004_CMD_FUNCTION_SET);
    lcd2004Command(LCD2004_CMD_DISPLAY_ON);
    lcd2004Command(LCD2004_CMD_CLEAR);
    lcd2004Command(LCD2004_CMD_ENTRY_MODE);
    lcd2004Command(LCD2004_CMD_HOME);
    lcd2004RenderStatus(LCD2004_STATUS_IDLE);
    lcd2004_available = true;

    PRINTF("LCD2004 ready on I2C0 PTB0/PTB1\r\n");
}

void lcd2004SetStatus(TLcd2004Status status)
{
    taskENTER_CRITICAL();
    current_status = status;
    taskEXIT_CRITICAL();
}

void lcd2004ResetToIdle(void)
{
    taskENTER_CRITICAL();
    current_status = LCD2004_STATUS_IDLE;
    taskEXIT_CRITICAL();
}

void lcd2004Task(void *p)
{
    TLcd2004Status last_status = (TLcd2004Status)(-1);

    (void)p;

    for (;;) {
        TLcd2004Status status = current_status;

        if (lcd2004_available && (status != last_status)) {
            lcd2004RenderStatus(status);
            last_status = status;
        }

        lcd2004DelayMs(LCD2004_STATUS_POLL_MS);
    }
}
