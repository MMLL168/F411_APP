#include "ssd1306.h"
#include "fonts.h"

/* 私有變數 */
static SSD1306_t SSD1306;
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];

/* 外部I2C控制結構 */
extern I2C_HandleTypeDef hi2c1;

/**
 * @brief  初始化SSD1306 OLED顯示器
 * @retval 初始化狀態 (1: 成功, 0: 失敗)
 */
#if 1
uint8_t ssd1306_Init(void)
{
    /* 等待100ms讓SSD1306開機 */
    HAL_Delay(100);

    /* 初始化LCD */
    ssd1306_WriteCommand(0xAE); //display off
    ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(0x10); // 00,Horizontal Addressing Mode; 01,Vertical Addressing Mode;
                                // 10,Page Addressing Mode (RESET); 11,Invalid
    ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction
    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address
    ssd1306_WriteCommand(0x40); //--set start line address - CHECK
    ssd1306_WriteCommand(0x81); //--set contrast control register - CHECK
    ssd1306_WriteCommand(0xFF);
    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK
    ssd1306_WriteCommand(0xA6); //--set normal color
    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK
    ssd1306_WriteCommand(0x1F); // 改為 0x1F (32-1) 而不是 0x3F (64-1)
    ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    ssd1306_WriteCommand(0xD3); //--set display offset - CHECK
    ssd1306_WriteCommand(0x00); //--not offset
    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--set divide ratio
    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //
    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
    ssd1306_WriteCommand(0x02); // 改為 0x02 (128x32用) 而不是 0x12 (128x64用)
    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc
    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    ssd1306_WriteCommand(0xAF); //--turn on SSD1306 panel

    /* 清除螢幕 */
    ssd1306_Fill(Black);

    /* 更新螢幕 */
    ssd1306_UpdateScreen();

    /* 設定預設值 */
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Initialized = 1;
    SSD1306.DisplayOn = 1;  // 添加這行

    /* 回傳OK */
    return 1;
}


#else //Old
uint8_t ssd1306_Init(void)
{
    /* 延遲讓顯示器準備好 */
    HAL_Delay(100);

    /* 初始化LCD */
    ssd1306_WriteCommand(SSD1306_DISPLAYOFF);                    // 0xAE
    ssd1306_WriteCommand(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_WriteCommand(0x80);                                  // 建議比例 0x80
    ssd1306_WriteCommand(SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_WriteCommand(SSD1306_HEIGHT - 1);
    ssd1306_WriteCommand(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_WriteCommand(0x0);                                   // 無偏移
    ssd1306_WriteCommand(SSD1306_SETSTARTLINE | 0x0);            // 行 #0
    ssd1306_WriteCommand(SSD1306_CHARGEPUMP);                    // 0x8D
    ssd1306_WriteCommand(0x14);                                  // 內部VCC
    ssd1306_WriteCommand(SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_WriteCommand(0x00);                                  // 0x0 水平定址模式
    ssd1306_WriteCommand(SSD1306_SEGREMAP | 0x1);
    ssd1306_WriteCommand(SSD1306_COMSCANDEC);

#if SSD1306_HEIGHT == 32
    ssd1306_WriteCommand(SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_WriteCommand(0x02);
    ssd1306_WriteCommand(SSD1306_SETCONTRAST);                   // 0x81
    ssd1306_WriteCommand(0x8F);
#elif SSD1306_HEIGHT == 64
    ssd1306_WriteCommand(SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_WriteCommand(0x12);
    ssd1306_WriteCommand(SSD1306_SETCONTRAST);                   // 0x81
    ssd1306_WriteCommand(0xCF);
#else
#error "只支援 32 或 64 像素高度的顯示器"
#endif

    ssd1306_WriteCommand(SSD1306_SETPRECHARGE);                  // 0xD9
    ssd1306_WriteCommand(0xF1);
    ssd1306_WriteCommand(SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_WriteCommand(0x40);
    ssd1306_WriteCommand(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_WriteCommand(SSD1306_NORMALDISPLAY);                 // 0xA6

    ssd1306_WriteCommand(SSD1306_DEACTIVATE_SCROLL);

    ssd1306_WriteCommand(SSD1306_DISPLAYON);                     // 開啟OLED面板

    /* 清空螢幕 */
    ssd1306_Fill(Black);

    /* 更新螢幕 */
    ssd1306_UpdateScreen();

    /* 設定預設值 */
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Initialized = 1;

    return 1;
}
#endif
/**
 * @brief  用指定顏色填滿整個螢幕
 * @param  color: 填充顏色
 * @retval None
 */
void ssd1306_Fill(SSD1306_COLOR color)
{
    uint32_t i;

    for(i = 0; i < sizeof(SSD1306_Buffer); i++)
    {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

/**
 * @brief  將緩衝區內容寫入螢幕
 * @retval None
 */
void ssd1306_UpdateScreen(void)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        ssd1306_WriteCommand(0xB0 + i); // 設定頁地址
        ssd1306_WriteCommand(0x00);     // 設定低欄位起始地址
        ssd1306_WriteCommand(0x10);     // 設定高欄位起始地址

        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
}

/**
 * @brief  在指定位置畫一個像素
 * @param  x: X座標
 * @param  y: Y座標
 * @param  color: 像素顏色
 * @retval None
 */
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    {
        return;
    }

    if (SSD1306.Inverted)
    {
        color = (SSD1306_COLOR)!color;
    }

    if (color == White)
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    }
    else
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/**
 * @brief  在當前位置寫入一個字元
 * @param  ch: 要寫入的字元
 * @param  Font: 字型指標
 * @param  color: 字元顏色
 * @retval 寫入的字元
 */
extern const uint8_t Font7x10_Data[];
char ssd1306_WriteChar(char ch, FontDef_t Font, SSD1306_COLOR color)
{
    uint32_t i, b, j;

    // 檢查字符範圍 - 擴大支持範圍
    if (ch < 32 || ch > 126) {
        return 0;  // 不支持的字符
    }

    // 檢查邊界
    if (SSD1306.CurrentX + Font.width >= SSD1306_WIDTH ||
        SSD1306.CurrentY + Font.height >= SSD1306_HEIGHT) {
        return 0;
    }

    // 計算字符在字體數據中的偏移
    uint32_t char_index = ch - 32;  // ASCII 32 是空格

    // 遍歷字體的每一行
    for (i = 0; i < Font.height; i++) {
        // 獲取當前行的字體數據
        b = Font.data[char_index * Font.height + i];

        // 遍歷當前行的每一位
        for (j = 0; j < Font.width; j++) {
            if ((b << j) & 0x80) {  // 檢查最高位
                ssd1306_DrawPixel(SSD1306.CurrentX + j,
                                SSD1306.CurrentY + i,
                                (SSD1306_COLOR) color);
            } else {
                ssd1306_DrawPixel(SSD1306.CurrentX + j,
                                SSD1306.CurrentY + i,
                                (SSD1306_COLOR)!color);
            }
        }
    }

    // 移動游標到下一個字符位置
    SSD1306.CurrentX += Font.width;

    // 檢查是否需要換行
    if (SSD1306.CurrentX + Font.width >= SSD1306_WIDTH) {
        SSD1306.CurrentX = 0;
        SSD1306.CurrentY += Font.height;

        // 如果超出螢幕高度，回到頂部
        if (SSD1306.CurrentY + Font.height >= SSD1306_HEIGHT) {
            SSD1306.CurrentY = 0;
        }
    }

    return ch;
}

/**
 * @brief  測試所有字符顯示
 * @retval None
 */
void ssd1306_TestAllCharacters(void)
{
    printf("=== SSD1306 字符顯示測試開始 ===\r\n");

    // 確保顯示器已初始化
    if (!SSD1306.Initialized) {
        printf("錯誤：SSD1306 未初始化\r\n");
        return;
    }

    // **測試1：基本連接測試**
    printf("1. 基本連接測試...\r\n");
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("SSD1306 OK!", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(2000);

    // **測試2：大寫字母 A-Z**
    printf("2. 測試大寫字母 A-Z...\r\n");
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("ABCDEFGHIJKLM", Font_7x10, White);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString("NOPQRSTUVWXYZ", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(3000);

    // **測試3：小寫字母 a-z**
    printf("3. 測試小寫字母 a-z...\r\n");
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("abcdefghijklm", Font_7x10, White);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString("nopqrstuvwxyz", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(3000);

    // **測試4：數字 0-9**
    printf("4. 測試數字 0-9...\r\n");
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("0123456789", Font_7x10, White);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString("9876543210", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(3000);

    // **測試5：標點符號**
    printf("5. 測試標點符號...\r\n");
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("!@#$%^&*()", Font_7x10, White);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString("[]{}|:;\"'<>", Font_7x10, White);
    ssd1306_SetCursor(0, 24);
    ssd1306_WriteString("?/.,`~+-=_", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(3000);

    printf("=== 字符顯示測試完成 ===\r\n");
}

/**
 * @brief  逐個測試ASCII字符
 * @retval None
 */
void ssd1306_TestIndividualChars(void)
{
    printf("=== 逐個ASCII字符測試 ===\r\n");

    char display_char[2] = {0, 0};  // 單字符字串
    char info_line[20];

    // 測試 ASCII 32-126 的所有可顯示字符
    for (int ascii = 32; ascii <= 126; ascii++) {
        ssd1306_Fill(Black);

        // 第一行：顯示ASCII碼
        ssd1306_SetCursor(0, 0);
        sprintf(info_line, "ASCII: %d", ascii);
        ssd1306_WriteString(info_line, Font_7x10, White);

        // 第二行：顯示字符描述
        ssd1306_SetCursor(0, 12);
        sprintf(info_line, "Char: '%c'", (char)ascii);
        ssd1306_WriteString(info_line, Font_7x10, White);

        // 第三行：大字顯示該字符
        ssd1306_SetCursor(50, 24);
        display_char[0] = (char)ascii;
        ssd1306_WriteString(display_char, Font_7x10, White);

        ssd1306_UpdateScreen();

        printf("顯示字符: '%c' (ASCII: %d)\r\n", (char)ascii, ascii);
        HAL_Delay(300);  // 每個字符顯示 0.3 秒
    }

    printf("=== 逐個字符測試完成 ===\r\n");
}

/**
 * @brief  測試字體大小（如果有多種字體）
 * @retval None
 */
void ssd1306_TestFonts(void)
{
    printf("=== 字體測試 ===\r\n");

    // 測試 Font_7x10
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Font 7x10:", Font_7x10, White);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString("ABC abc 123", Font_7x10, White);
    ssd1306_UpdateScreen();
    HAL_Delay(2000);

    // 如果有其他字體，可以在這裡測試
    // ssd1306_Fill(Black);
    // ssd1306_SetCursor(0, 0);
    // ssd1306_WriteString("Font 11x18:", Font_7x10, White);
    // ssd1306_SetCursor(0, 15);
    // ssd1306_WriteString("ABC 123", Font_11x18, White);
    // ssd1306_UpdateScreen();
    // HAL_Delay(2000);

    printf("=== 字體測試完成 ===\r\n");
}

/**
 * @brief  持續顯示測試
 * @retval None
 */
void ssd1306_ContinuousTest(void)
{
    printf("=== 持續顯示測試開始 ===\r\n");

    uint32_t counter = 0;
    char line1[20], line2[20], line3[20];

    while (1) {
        ssd1306_Fill(Black);

        // 第一行：計數器
        ssd1306_SetCursor(0, 0);
        sprintf(line1, "Count: %lu", counter);
        ssd1306_WriteString(line1, Font_7x10, White);

        // 第二行：時間
        ssd1306_SetCursor(0, 12);
        sprintf(line2, "Time: %lu", HAL_GetTick() / 1000);
        ssd1306_WriteString(line2, Font_7x10, White);

        // 第三行：循環字符
        ssd1306_SetCursor(0, 24);
        sprintf(line3, "Test: %c%d", 'A' + (counter % 26), counter % 10);
        ssd1306_WriteString(line3, Font_7x10, White);

        ssd1306_UpdateScreen();

        counter++;
        HAL_Delay(1000);

        // 每 10 次輸出一次狀態
        if (counter % 10 == 0) {
            printf("持續測試中... Count: %lu\r\n", counter);
        }

        // 測試 100 次後停止
        if (counter >= 100) {
            break;
        }
    }

    printf("=== 持續顯示測試完成 ===\r\n");
}



/**
 * @brief  在當前位置寫入字串
 * @param  str: 要寫入的字串
 * @param  Font: 字型指標
 * @param  color: 字串顏色
 * @retval 最後寫入的字元
 */
void ssd1306_WriteString(char* str, FontDef_t Font, SSD1306_COLOR color)  // 修正：使用 FontDef_t
{
    /* 寫入字串直到結束符 */
    while (*str) {
        if (ssd1306_WriteChar(*str, Font, color) != *str) {
            /* 字元寫入失敗 */
            return;
        }
        str++;
    }
}

/**
 * @brief  設定游標位置
 * @param  x: X座標
 * @param  y: Y座標
 * @retval None
 */
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

/**
 * @brief  畫直線
 * @param  x1, y1: 起點座標
 * @param  x2, y2: 終點座標
 * @param  color: 線條顏色
 * @retval None
 */
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color)
{
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;

    while (1)
    {
        ssd1306_DrawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;
        error2 = error * 2;
        if (error2 > -deltaY)
        {
            error -= deltaY;
            x1 += signX;
        }

        if (error2 < deltaX)
        {
            error += deltaX;
            y1 += signY;
        }
    }
}

/**
 * @brief  畫矩形
 * @param  x1, y1: 左上角座標
 * @param  x2, y2: 右下角座標
 * @param  color: 矩形顏色
 * @retval None
 */
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color)
{
    ssd1306_Line(x1, y1, x2, y1, color);
    ssd1306_Line(x2, y1, x2, y2, color);
    ssd1306_Line(x2, y2, x1, y2, color);
    ssd1306_Line(x1, y2, x1, y1, color);
}

/**
 * @brief  畫圓
 * @param  par_x, par_y: 圓心座標
 * @param  par_r: 半徑
 * @param  color: 圓的顏色
 * @retval None
 */
void ssd1306_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR color)
{
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT)
    {
        return;
    }

    do {
        ssd1306_DrawPixel(par_x - x, par_y + y, color);
        ssd1306_DrawPixel(par_x + x, par_y + y, color);
        ssd1306_DrawPixel(par_x + x, par_y - y, color);
        ssd1306_DrawPixel(par_x - x, par_y - y, color);
        e2 = err;
        if (e2 <= y)
        {
            y++;
            err = err + (y * 2 + 1);
            if(-x == y && e2 <= x)
            {
                e2 = 0;
            }
        }
        if (e2 > x)
        {
            x++;
            err = err + (x * 2 + 1);
        }
    } while (x <= 0);
}

/**
 * @brief  畫點陣圖
 * @param  x, y: 起始座標
 * @param  bitmap: 點陣圖數據
 * @param  w: 寬度
 * @param  h: 高度
 * @param  color: 顏色
 * @retval None
 */
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color)
{
    int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;

    for (int16_t j = 0; j < h; j++, y++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            if (i & 7)
            {
                byte <<= 1;
            }
            else
            {
                byte = bitmap[j * byteWidth + i / 8];
            }
            if (byte & 0x80)
            {
                ssd1306_DrawPixel(x + i, y, color);
            }
        }
    }
}

/**
 * @brief  設定對比度
 * @param  value: 對比度值 (0-255)
 * @retval None
 */
void ssd1306_SetContrast(const uint8_t value)
{
    ssd1306_WriteCommand(SSD1306_SETCONTRAST);
    ssd1306_WriteCommand(value);
}

/**
 * @brief  設定顯示開/關
 * @param  on: 1=開啟, 0=關閉
 * @retval None
 */
void ssd1306_SetDisplayOn(const uint8_t on)
{
    uint8_t value;
    if (on)
    {
        value = SSD1306_DISPLAYON;
        SSD1306.DisplayOn = 1;
    }
    else
    {
        value = SSD1306_DISPLAYOFF;
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(value);
}

/**
 * @brief  取得顯示狀態
 * @retval 顯示狀態 (1=開啟, 0=關閉)
 */
uint8_t ssd1306_GetDisplayOn(void)
{
    return SSD1306.DisplayOn;
}

/**
 * @brief  反轉顯示
 * @param  i: 1=反轉, 0=正常
 * @retval None
 */
void ssd1306_InvertDisplay(int i)
{
    if (i)
    {
        ssd1306_WriteCommand(SSD1306_INVERTDISPLAY);
    }
    else
    {
        ssd1306_WriteCommand(SSD1306_NORMALDISPLAY);
    }
}

/**
 * @brief  寫入命令到SSD1306
 * @param  byte: 命令位元組
 * @retval None
 */
void ssd1306_WriteCommand(uint8_t byte)
{
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

/**
 * @brief  寫入數據到SSD1306
 * @param  buffer: 數據緩衝區
 * @param  buff_size: 緩衝區大小
 * @retval None
 */
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size)
{
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

/**
 * @brief  開啟顯示器
 * @retval None
 */
void ssd1306_ON(void) {
    ssd1306_WriteCommand(SSD1306_DISPLAYON);
    SSD1306.DisplayOn = 1;
}

/**
 * @brief  關閉顯示器
 * @retval None
 */
void ssd1306_OFF(void) {
    ssd1306_WriteCommand(SSD1306_DISPLAYOFF);
    SSD1306.DisplayOn = 0;
}

/**
 * @brief  向右滾動
 * @param  start_row: 開始行
 * @param  end_row: 結束行
 * @retval None
 */
void ssd1306_ScrollRight(uint8_t start_row, uint8_t end_row) {
    ssd1306_WriteCommand(SSD1306_RIGHT_HORIZONTAL_SCROLL);  // send 0x26
    ssd1306_WriteCommand(0x00);  // send dummy
    ssd1306_WriteCommand(start_row);  // start page address
    ssd1306_WriteCommand(0X00);  // time interval 5 frames
    ssd1306_WriteCommand(end_row);  // end page address
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(0XFF);
    ssd1306_WriteCommand(SSD1306_ACTIVATE_SCROLL); // start scroll
}

/**
 * @brief  向左滾動
 * @param  start_row: 開始行
 * @param  end_row: 結束行
 * @retval None
 */
void ssd1306_ScrollLeft(uint8_t start_row, uint8_t end_row) {
    ssd1306_WriteCommand(SSD1306_LEFT_HORIZONTAL_SCROLL);  // send 0x27
    ssd1306_WriteCommand(0x00);  // send dummy
    ssd1306_WriteCommand(start_row);  // start page address
    ssd1306_WriteCommand(0X00);  // time interval 5 frames
    ssd1306_WriteCommand(end_row);  // end page address
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(0XFF);
    ssd1306_WriteCommand(SSD1306_ACTIVATE_SCROLL); // start scroll
}

/**
 * @brief  對角線向右滾動
 * @param  start_row: 開始行
 * @param  end_row: 結束行
 * @retval None
 */
void ssd1306_Scrolldiagright(uint8_t start_row, uint8_t end_row) {
    ssd1306_WriteCommand(SSD1306_SET_VERTICAL_SCROLL_AREA);
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(SSD1306_HEIGHT);
    ssd1306_WriteCommand(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(start_row);
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(end_row);
    ssd1306_WriteCommand(0X01);
    ssd1306_WriteCommand(SSD1306_ACTIVATE_SCROLL);
}

/**
 * @brief  對角線向左滾動
 * @param  start_row: 開始行
 * @param  end_row: 結束行
 * @retval None
 */
void ssd1306_Scrolldiagleft(uint8_t start_row, uint8_t end_row) {
    ssd1306_WriteCommand(SSD1306_SET_VERTICAL_SCROLL_AREA);
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(SSD1306_HEIGHT);
    ssd1306_WriteCommand(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(start_row);
    ssd1306_WriteCommand(0X00);
    ssd1306_WriteCommand(end_row);
    ssd1306_WriteCommand(0X01);
    ssd1306_WriteCommand(SSD1306_ACTIVATE_SCROLL);
}

/**
 * @brief  停止滾動
 * @retval None
 */
void ssd1306_Stopscroll(void) {
    ssd1306_WriteCommand(SSD1306_DEACTIVATE_SCROLL);
}

