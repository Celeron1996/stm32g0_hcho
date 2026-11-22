#include "oled.h"
#include "oledfont.h"

#include <string.h>

#define OLED_I2C_ADDRESS   (0x3CU << 1)
#define OLED_I2C_TIMEOUT   1000U

static I2C_HandleTypeDef *oled_i2c;
static uint8_t oled_buffer[OLED_HEIGHT / 8U][OLED_WIDTH];
static uint8_t oled_tx_buffer[OLED_WIDTH + 1U];

static HAL_StatusTypeDef OLED_WriteCommandList(const uint8_t *commands, uint16_t length)
{
  uint8_t buffer[2];

  for (uint16_t i = 0U; i < length; i++)
  {
    buffer[0] = OLED_CMD;
    buffer[1] = commands[i];
    if (HAL_I2C_Master_Transmit(oled_i2c, OLED_I2C_ADDRESS, buffer, 2U,
                                OLED_I2C_TIMEOUT) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c)
{
  static const uint8_t init_commands[] =
  {
    0xAE,
    0xD5, 0x80,
    0xA8, 0x3F,
    0xD3, 0x00,
    0x40,
    0x8D, 0x14,
    0x20, 0x02,
    0xA1,
    0xC8,
    0xDA, 0x12,
    0x81, 0xB0,
    0xD9, 0x22,
    0xDB, 0x30,
    0xA4,
    0xA6,
    0x2E,
    0xAF
  };

  if (hi2c == NULL)
  {
    return HAL_ERROR;
  }

  oled_i2c = hi2c;

  if (OLED_WriteCommandList(init_commands, sizeof(init_commands)) != HAL_OK)
  {
    return HAL_ERROR;
  }

  OLED_ClearBuffer();
  OLED_Refresh();

  return HAL_OK;
}

void OLED_ClearBuffer(void)
{
  memset(oled_buffer, 0x00, sizeof(oled_buffer));
}

void OLED_ClearPages(uint8_t first_page, uint8_t last_page)
{
  if (last_page >= (OLED_HEIGHT / 8U))
  {
    last_page = (OLED_HEIGHT / 8U) - 1U;
  }

  for (uint8_t page = first_page; page <= last_page; page++)
  {
    memset(oled_buffer[page], 0x00, OLED_WIDTH);
  }
}

void OLED_Refresh(void)
{
  OLED_RefreshPages(0U, (OLED_HEIGHT / 8U) - 1U);
}

void OLED_RefreshPages(uint8_t first_page, uint8_t last_page)
{
  uint8_t page_commands[3];

  if (oled_i2c == NULL)
  {
    return;
  }

  if (last_page >= (OLED_HEIGHT / 8U))
  {
    last_page = (OLED_HEIGHT / 8U) - 1U;
  }

  for (uint8_t page = first_page; page <= last_page; page++)
  {
    page_commands[0] = (uint8_t)(0xB0U | page);
    page_commands[1] = 0x00U;
    page_commands[2] = 0x10U;
    (void)OLED_WriteCommandList(page_commands, sizeof(page_commands));

    oled_tx_buffer[0] = OLED_DATA;
    memcpy(&oled_tx_buffer[1], oled_buffer[page], OLED_WIDTH);
    (void)HAL_I2C_Master_Transmit(oled_i2c, OLED_I2C_ADDRESS, oled_tx_buffer,
                                  sizeof(oled_tx_buffer), OLED_I2C_TIMEOUT);
  }
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
  if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT))
  {
    return;
  }

  if (color != 0U)
  {
    oled_buffer[y >> 3][x] |= (uint8_t)(1U << (y & 0x07U));
  }
  else
  {
    oled_buffer[y >> 3][x] &= (uint8_t)~(1U << (y & 0x07U));
  }
}

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
  const uint8_t *font;
  uint8_t width;
  uint8_t bytes;
  uint8_t scale = 1U;
  uint8_t index;

  if ((ch < ' ') || (ch > '~'))
  {
    ch = '?';
  }
  index = (uint8_t)(ch - ' ');

  if (size == OLED_FONT_6X8)
  {
    font = asc2_0806[index];
    width = 6U;
    bytes = 6U;
  }
  else if (size == OLED_FONT_8X16)
  {
    font = asc2_1608[index];
    width = 8U;
    bytes = 16U;
  }
  else if (size == OLED_FONT_12X24)
  {
    font = asc2_2412[index];
    width = 12U;
    bytes = 36U;
  }
  else if (size == OLED_FONT_24X48)
  {
    font = asc2_2412[index];
    width = 12U;
    bytes = 36U;
    scale = 2U;
  }
  else
  {
    return;
  }

  for (uint8_t i = 0U; i < bytes; i++)
  {
    uint8_t column = (uint8_t)(i % width);
    uint8_t page = (uint8_t)(i / width);
    uint8_t data = font[i];

    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
      for (uint8_t dy = 0U; dy < scale; dy++)
      {
        for (uint8_t dx = 0U; dx < scale; dx++)
        {
          OLED_DrawPoint((uint8_t)(x + column * scale + dx),
                         (uint8_t)(y + (page * 8U + bit) * scale + dy),
                         (uint8_t)((data >> bit) & 0x01U));
        }
      }
    }
  }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
  uint8_t char_width;

  if (size == OLED_FONT_6X8)
  {
    char_width = 6U;
  }
  else if ((size == OLED_FONT_8X16) || (size == OLED_FONT_12X24))
  {
    char_width = (uint8_t)(size / 2U);
  }
  else if (size == OLED_FONT_24X48)
  {
    char_width = 24U;
  }
  else
  {
    return;
  }

  while ((str != NULL) && (*str != '\0'))
  {
    OLED_ShowChar(x, y, *str, size);
    x = (uint8_t)(x + char_width);
    str++;
  }
}

uint8_t OLED_StringWidth(const char *str, uint8_t size)
{
  uint8_t char_width;

  if (size == OLED_FONT_6X8)
  {
    char_width = 6U;
  }
  else if ((size == OLED_FONT_8X16) || (size == OLED_FONT_12X24))
  {
    char_width = (uint8_t)(size / 2U);
  }
  else if (size == OLED_FONT_24X48)
  {
    char_width = 24U;
  }
  else
  {
    return 0U;
  }

  if (str == NULL)
  {
    return 0U;
  }

  return (uint8_t)(strlen(str) * char_width);
}
