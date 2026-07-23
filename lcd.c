#include "lcd.h"
#include "ti_msp_dl_config.h"
static void byte(uint8_t x){uint8_t i;for(i=0;i<8;i++){if(x&0x80)DL_GPIO_setPins(GPIOB,LCD_SERIAL_LCD_SDI_PIN);else DL_GPIO_clearPins(GPIOB,LCD_SERIAL_LCD_SDI_PIN);DL_GPIO_setPins(GPIOA,LCD_PORTA_LCD_CLK_PIN);DL_GPIO_clearPins(GPIOA,LCD_PORTA_LCD_CLK_PIN);x<<=1;}}
static void cmd(uint8_t x){DL_GPIO_clearPins(GPIOB,LCD_CTRL_LCD_DC_PIN|LCD_SERIAL_LCD_CS_PIN);byte(x);DL_GPIO_setPins(GPIOB,LCD_SERIAL_LCD_CS_PIN);} static void dat(uint8_t x){DL_GPIO_setPins(GPIOB,LCD_CTRL_LCD_DC_PIN);DL_GPIO_clearPins(GPIOB,LCD_SERIAL_LCD_CS_PIN);byte(x);DL_GPIO_setPins(GPIOB,LCD_SERIAL_LCD_CS_PIN);}
static void area(uint16_t x,uint16_t y,uint16_t xe,uint16_t ye){cmd(0x2A);dat(x>>8);dat(x);dat(xe>>8);dat(xe);cmd(0x2B);dat(y>>8);dat(y);dat(ye>>8);dat(ye);cmd(0x2C);}
static void pixel(uint16_t c){dat(c>>8);dat(c);} static void fill(uint16_t c){uint32_t i;area(0,0,239,319);for(i=0;i<76800;i++)pixel(c);}
void Lcd_init(void){DL_GPIO_setPins(GPIOA,LCD_PORTA_LCD_BLK_PIN);cmd(0x01);delay_cycles(320000);cmd(0x11);delay_cycles(320000);cmd(0x3A);dat(0x55);cmd(0x36);dat(0x48);cmd(0x29);fill(0x0000);}
/* Seven-segment characters keep the display independent of a large font table. */
static void bar(uint8_t x,uint8_t y,uint8_t h,uint8_t v){uint8_t i,j;for(i=0;i<h;i++)for(j=0;j<v;j++){area(x+j,y+i,x+j,y+i);pixel(0xFFFF);}}
static void digit(uint8_t x,uint8_t y,uint8_t d){static const uint8_t m[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};uint8_t a=m[d];if(a&1)bar(x+2,y,3,14);if(a&2)bar(x+16,y+2,14,3);if(a&4)bar(x+16,y+18,14,3);if(a&8)bar(x+2,y+32,3,14);if(a&16)bar(x,y+18,14,3);if(a&32)bar(x,y+2,14,3);if(a&64)bar(x+2,y+16,3,14);}
void Lcd_showYaw(int16_t d){uint16_t a=(d<0)?(uint16_t)-d:(uint16_t)d;fill(0);/* YAW title is intentionally omitted: numeric angle is unambiguous */ if(d<0)bar(8,40,3,14); digit(40,40,(a/1000)%10);digit(72,40,(a/100)%10);digit(104,40,(a/10)%10);bar(136,70,5,5);digit(152,40,a%10);}
