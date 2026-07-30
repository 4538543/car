#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define LCD_WIDTH                         (320U)
#define LCD_HEIGHT                        (240U)

#define LCD_COLOR_BLACK                   (0x0000U)
#define LCD_COLOR_WHITE                   (0xFFFFU)
#define LCD_COLOR_RED                     (0xF800U)
#define LCD_COLOR_GREEN                   (0x07E0U)
#define LCD_COLOR_BLUE                    (0x001FU)
#define LCD_COLOR_YELLOW                  (0xFFE0U)
#define LCD_COLOR_CYAN                    (0x07FFU)
#define LCD_COLOR_DARK_BLUE               (0x1082U)
#define LCD_COLOR_DARK_GRAY               (0x3186U)

void Lcd_init(void);
void Lcd_fillScreen(uint16_t color);
void Lcd_fillRect(
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void Lcd_drawText(uint16_t x, uint16_t y, const char *text,
    uint8_t scale, uint16_t foreground, uint16_t background);

#endif
