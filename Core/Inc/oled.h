#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g0xx_hal.h"

#define OLED_WIDTH   128U
#define OLED_HEIGHT  64U

#define OLED_CMD     0x00U
#define OLED_DATA    0x40U

#define OLED_FONT_6X8   8U
#define OLED_FONT_8X16  16U
#define OLED_FONT_12X24 24U
#define OLED_FONT_24X48 48U

HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_ClearBuffer(void);
void OLED_ClearPages(uint8_t first_page, uint8_t last_page);
void OLED_Refresh(void);
void OLED_RefreshPages(uint8_t first_page, uint8_t last_page);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
uint8_t OLED_StringWidth(const char *str, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif
