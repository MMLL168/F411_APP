#ifndef __SSD1306_H
#define __SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "fonts.h"
#include <string.h>
#include <stdlib.h>

/* SSD1306 設定 */
#define SSD1306_I2C_PORT        hi2c1
#define SSD1306_I2C_ADDR        (0x3C << 1)  // 0x3C 左移1位 = 0x78

/* 螢幕尺寸 */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          32
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

#define OLED_WIDTH  128
#define OLED_HEIGHT 32

/* SSD1306 命令 */
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON        0xA5
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_INVERTDISPLAY       0xA7
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_SETLOWCOLUMN        0x00
#define SSD1306_SETHIGHCOLUMN       0x10
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22
#define SSD1306_COMSCANINC          0xC0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_EXTERNALVCC         0x1
#define SSD1306_SWITCHCAPVCC        0x2

/* 滾動命令 */
#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL  0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL   0x2A
#define SSD1306_DEACTIVATE_SCROLL                     0x2E
#define SSD1306_ACTIVATE_SCROLL                       0x2F
#define SSD1306_SET_VERTICAL_SCROLL_AREA              0xA3

/* 顏色列舉 */
typedef enum {
    Black = 0x00, /*!< 黑色，像素關閉 */
    White = 0x01  /*!< 白色，像素開啟 */
} SSD1306_COLOR;

/* SSD1306 結構 - 添加 DisplayOn 成員 */
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
    uint8_t DisplayOn;  // 添加這個成員
} SSD1306_t;

/* 函數宣告 */
uint8_t ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
char ssd1306_WriteChar(char ch, FontDef_t Font, SSD1306_COLOR color);
void ssd1306_WriteString(char* str, FontDef_t Font, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);

/* 顯示控制函數 */
void ssd1306_ON(void);
void ssd1306_OFF(void);

/* 滾動函數 */
void ssd1306_ScrollRight(uint8_t start_row, uint8_t end_row);
void ssd1306_ScrollLeft(uint8_t start_row, uint8_t end_row);
void ssd1306_Scrolldiagright(uint8_t start_row, uint8_t end_row);
void ssd1306_Scrolldiagleft(uint8_t start_row, uint8_t end_row);
void ssd1306_Stopscroll(void);

/* 低層函數 */
void ssd1306_WriteCommand(uint8_t byte);
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size);
// 在 ssd1306.h 中添加
void ssd1306_TestAllCharacters(void);
void ssd1306_TestIndividualChars(void);
void ssd1306_TestFonts(void);
void ssd1306_ContinuousTest(void);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H */
