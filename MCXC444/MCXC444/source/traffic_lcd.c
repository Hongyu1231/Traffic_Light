#include "traffic_lcd.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_context.h"
#include "board.h"
#include "fsl_common.h"
#include "FreeRTOS.h"
#include "task.h"

/* Raw register access for the onboard segmented LCD. */
#define REG32(addr) (*(volatile uint32_t *)(addr))
#define REG8(addr)  (*(volatile uint8_t *)(addr))

#define SIM_SCGC5_REG   REG32(0x40048038u)
#define MCG_C1_REG      REG8(0x40064000u)
#define MCG_C2_REG      REG8(0x40064001u)

#define LCD_GCR_REG     REG32(0x40053000u)
#define LCD_AR_REG      REG32(0x40053004u)
#define LCD_PENL_REG    REG32(0x40053010u)
#define LCD_PENH_REG    REG32(0x40053014u)
#define LCD_BPENL_REG   REG32(0x40053018u)
#define LCD_BPENH_REG   REG32(0x4005301Cu)
#define LCD_WF_BASE     (0x40053020u)

#define LCD_PIN_COM0   59u
#define LCD_PIN_COM1   60u
#define LCD_PIN_COM2   14u
#define LCD_PIN_COM3   15u

#define LCD_PIN_D0     20u
#define LCD_PIN_D1     24u
#define LCD_PIN_D2     26u
#define LCD_PIN_D3     27u
#define LCD_PIN_D4     40u
#define LCD_PIN_D5     42u
#define LCD_PIN_D6     43u
#define LCD_PIN_D7     44u

#define SEG_A   (1u << 0)
#define SEG_B   (1u << 1)
#define SEG_C   (1u << 2)
#define SEG_D   (1u << 3)
#define SEG_E   (1u << 4)
#define SEG_F   (1u << 5)
#define SEG_G   (1u << 6)

static const uint8_t digit_seg_map[10] =
{
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_B | SEG_C,
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
    SEG_B | SEG_C | SEG_F | SEG_G,
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G
};

static inline void lcdWaveformWrite(uint8_t pin, uint8_t value)
{
    REG8(LCD_WF_BASE + pin) = value;
}

static void lcdClearFrontplanes(void)
{
    lcdWaveformWrite(LCD_PIN_D0, 0x00);
    lcdWaveformWrite(LCD_PIN_D1, 0x00);
    lcdWaveformWrite(LCD_PIN_D2, 0x00);
    lcdWaveformWrite(LCD_PIN_D3, 0x00);
    lcdWaveformWrite(LCD_PIN_D4, 0x00);
    lcdWaveformWrite(LCD_PIN_D5, 0x00);
    lcdWaveformWrite(LCD_PIN_D6, 0x00);
    lcdWaveformWrite(LCD_PIN_D7, 0x00);
}

static void lcdSetDigit(uint8_t digit_index, uint8_t segs)
{
    uint8_t pin_col0;
    uint8_t pin_col1;
    uint8_t col0 = 0;
    uint8_t col1 = 0;

    switch (digit_index)
    {
        case 0:
            pin_col0 = LCD_PIN_D0;
            pin_col1 = LCD_PIN_D1;
            break;
        case 1:
            pin_col0 = LCD_PIN_D2;
            pin_col1 = LCD_PIN_D3;
            break;
        case 2:
            pin_col0 = LCD_PIN_D4;
            pin_col1 = LCD_PIN_D5;
            break;
        case 3:
        default:
            pin_col0 = LCD_PIN_D6;
            pin_col1 = LCD_PIN_D7;
            break;
    }

    if ((segs & SEG_D) != 0u) { col0 |= 0x01; }
    if ((segs & SEG_E) != 0u) { col0 |= 0x02; }
    if ((segs & SEG_G) != 0u) { col0 |= 0x04; }
    if ((segs & SEG_F) != 0u) { col0 |= 0x08; }

    if ((segs & SEG_C) != 0u) { col1 |= 0x02; }
    if ((segs & SEG_B) != 0u) { col1 |= 0x04; }
    if ((segs & SEG_A) != 0u) { col1 |= 0x08; }

    lcdWaveformWrite(pin_col0, col0);
    lcdWaveformWrite(pin_col1, col1);
}

static void lcdBlankDisplay(void)
{
    lcdClearFrontplanes();
}

static void lcdShowSingleDigit(uint8_t value)
{
    lcdBlankDisplay();
    lcdSetDigit(3u, digit_seg_map[value]);
}

void initSLCD(void)
{
    uint32_t penl = 0u;
    uint32_t penh = 0u;
    uint32_t bpenl = 0u;
    uint32_t bpenh = 0u;
    uint32_t gcr_cfg;

    SIM_SCGC5_REG |= SIM_SCGC5_SLCD_MASK;
    MCG_C2_REG |= MCG_C2_IRCS_MASK;
    MCG_C1_REG |= MCG_C1_IRCLKEN_MASK;

    penl |= (1u << LCD_PIN_COM2);
    penl |= (1u << LCD_PIN_COM3);
    penl |= (1u << LCD_PIN_D0);
    penl |= (1u << LCD_PIN_D1);
    penl |= (1u << LCD_PIN_D2);
    penl |= (1u << LCD_PIN_D3);

    penh |= (1u << (LCD_PIN_D4 - 32u));
    penh |= (1u << (LCD_PIN_D5 - 32u));
    penh |= (1u << (LCD_PIN_D6 - 32u));
    penh |= (1u << (LCD_PIN_D7 - 32u));
    penh |= (1u << (LCD_PIN_COM0 - 32u));
    penh |= (1u << (LCD_PIN_COM1 - 32u));

    bpenl |= (1u << LCD_PIN_COM2);
    bpenl |= (1u << LCD_PIN_COM3);
    bpenh |= (1u << (LCD_PIN_COM0 - 32u));
    bpenh |= (1u << (LCD_PIN_COM1 - 32u));

    gcr_cfg =
        LCD_GCR_RVEN(0) |
        LCD_GCR_CPSEL(1) |
        LCD_GCR_LADJ(0) |
        LCD_GCR_VSUPPLY(0) |
        LCD_GCR_PADSAFE(0) |
        LCD_GCR_ALTDIV(3) |
        LCD_GCR_ALTSOURCE(0) |
        LCD_GCR_FFR(0) |
        LCD_GCR_LCDDOZE(0) |
        LCD_GCR_LCDSTP(0) |
        LCD_GCR_LCDEN(0) |
        LCD_GCR_SOURCE(1) |
        LCD_GCR_LCLK(0) |
        LCD_GCR_DUTY(3);

    LCD_GCR_REG = gcr_cfg;
    LCD_AR_REG = 0x00000000u;

    LCD_PENL_REG = penl;
    LCD_PENH_REG = penh;
    LCD_BPENL_REG = bpenl;
    LCD_BPENH_REG = bpenh;

    lcdWaveformWrite(LCD_PIN_COM0, 0x01);
    lcdWaveformWrite(LCD_PIN_COM1, 0x02);
    lcdWaveformWrite(LCD_PIN_COM2, 0x04);
    lcdWaveformWrite(LCD_PIN_COM3, 0x08);

    lcdBlankDisplay();
    LCD_GCR_REG = gcr_cfg | LCD_GCR_LCDEN(1);
}

void setLCDCountdownValue(int value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > 9) {
        value = 9;
    }

    lcd_countdown_seconds = value;
}

void lcdTask(void *p)
{
    int last_value = -1;

    (void)p;

    for (;;) {
        int current_value = lcd_countdown_seconds;

        if (current_value != last_value) {
            if (current_value >= 1) {
                lcdShowSingleDigit((uint8_t)current_value);
            } else {
                lcdBlankDisplay();
            }

            last_value = current_value;
        }

        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}
