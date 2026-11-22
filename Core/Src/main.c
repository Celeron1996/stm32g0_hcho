/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  UI_STATE_LOST = 0,
  UI_STATE_READING
} UI_State_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HCHO_FRAME_SIZE       9U
#define HCHO_FRAME_HEAD       0xFFU
#define HCHO_GAS_CODE         0x17U
#define HCHO_SIGNAL_TIMEOUT   3000U
#define OLED_I2C_TIMING       0x30414C4DU
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;

static uint8_t hcho_rx_byte;
static uint8_t hcho_frame[HCHO_FRAME_SIZE];
static uint8_t hcho_frame_index;
static volatile uint16_t hcho_value_ppb;
static volatile uint32_t hcho_last_frame_tick;
static volatile uint8_t hcho_data_valid;
static volatile uint8_t hcho_new_frame;

static UI_State_t ui_state;
static uint16_t ui_last_ppb;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void HCHO_Start(void);
static void HCHO_ProcessByte(uint8_t byte);
static void UI_ShowCenteredText(uint8_t y, const char *text, uint8_t size);
static void UI_ShowReading(uint16_t ppb);
static void UI_ShowMessage(const char *value, const char *message);
static uint8_t FormatMgPerM3(uint16_t ppb, char *buffer);
static uint8_t FormatU16(uint16_t value, char *buffer);
static void LogString(const char *text);
static void LogReading(uint16_t ppb);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  LogString("\r\nHCHO detector start\r\n");

  HAL_Delay(100);

  if (OLED_Init(&hi2c1) != HAL_OK)
  {
    LogString("OLED init failed\r\n");
  }

  UI_ShowMessage("---", "WAITING FOR DATA");
  ui_state = UI_STATE_LOST;
  ui_last_ppb = 0xFFFFU;

  HCHO_Start();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    uint32_t last_tick;
    uint16_t ppb;
    uint8_t valid;
    uint8_t fresh;

    __disable_irq();
    ppb = hcho_value_ppb;
    valid = hcho_data_valid;
    fresh = hcho_new_frame;
    last_tick = hcho_last_frame_tick;
    hcho_new_frame = 0U;
    __enable_irq();

    if ((valid == 0U) || ((now - last_tick) > HCHO_SIGNAL_TIMEOUT))
    {
      if (ui_state != UI_STATE_LOST)
      {
        if (valid == 0U)
        {
          UI_ShowMessage("---", "WAITING FOR DATA");
          LogString("waiting for sensor data\r\n");
        }
        else
        {
          UI_ShowMessage("---", "SENSOR NO SIGNAL");
          LogString("sensor timeout\r\n");
        }
        ui_state = UI_STATE_LOST;
      }
    }
    else if ((fresh != 0U) || (ui_state != UI_STATE_READING) ||
             (ppb != ui_last_ppb))
    {
      if (fresh != 0U)
      {
        LogReading(ppb);
      }

      if ((ui_state != UI_STATE_READING) || (ppb != ui_last_ppb))
      {
        UI_ShowReading(ppb);
        ui_last_ppb = ppb;
      }
      ui_state = UI_STATE_READING;
    }

    HAL_Delay(20);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = OLED_I2C_TIMING;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void HCHO_Start(void)
{
  hcho_frame_index = 0U;
  hcho_data_valid = 0U;
  hcho_new_frame = 0U;
  (void)HAL_UART_Receive_IT(&huart2, &hcho_rx_byte, 1U);
}

static void HCHO_ProcessByte(uint8_t byte)
{
  if (hcho_frame_index == 0U)
  {
    if (byte == HCHO_FRAME_HEAD)
    {
      hcho_frame[hcho_frame_index++] = byte;
    }
    return;
  }

  if (hcho_frame_index == 1U)
  {
    if (byte == HCHO_GAS_CODE)
    {
      hcho_frame[hcho_frame_index++] = byte;
    }
    else if (byte != HCHO_FRAME_HEAD)
    {
      hcho_frame_index = 0U;
    }
    return;
  }

  hcho_frame[hcho_frame_index++] = byte;

  if (hcho_frame_index >= HCHO_FRAME_SIZE)
  {
    uint8_t checksum = 0U;

    for (uint8_t i = 1U; i < HCHO_FRAME_SIZE; i++)
    {
      checksum = (uint8_t)(checksum + hcho_frame[i]);
    }

    if (checksum == 0U)
    {
      hcho_value_ppb = (uint16_t)(((uint16_t)hcho_frame[4] << 8) |
                                  hcho_frame[5]);
      hcho_last_frame_tick = HAL_GetTick();
      hcho_data_valid = 1U;
      hcho_new_frame = 1U;
    }

    hcho_frame_index = 0U;
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    HCHO_ProcessByte(hcho_rx_byte);
    (void)HAL_UART_Receive_IT(&huart2, &hcho_rx_byte, 1U);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    hcho_frame_index = 0U;
    (void)HAL_UART_Receive_IT(&huart2, &hcho_rx_byte, 1U);
  }
}

static void UI_ShowCenteredText(uint8_t y, const char *text, uint8_t size)
{
  uint8_t width = OLED_StringWidth(text, size);
  uint8_t x = (width < OLED_WIDTH) ? (uint8_t)((OLED_WIDTH - width) / 2U) : 0U;

  OLED_ShowString(x, y, text, size);
}

static void UI_ShowReading(uint16_t ppb)
{
  char value[8];
  char ppb_text[12];
  uint8_t value_length = FormatMgPerM3(ppb, value);
  uint8_t value_width = (uint8_t)(value_length * 24U);
  uint8_t x = (value_width < OLED_WIDTH) ?
              (uint8_t)((OLED_WIDTH - value_width) / 2U) : 0U;
  uint8_t text_length;
  uint8_t ppb_width;

  OLED_ClearBuffer();

  OLED_ShowString(x, 2U, value, OLED_FONT_24X48);
  OLED_ShowString(8U, 54U, "mg/m3", OLED_FONT_6X8);

  text_length = FormatU16(ppb, ppb_text);
  ppb_text[text_length++] = ' ';
  ppb_text[text_length++] = 'p';
  ppb_text[text_length++] = 'p';
  ppb_text[text_length++] = 'b';
  ppb_text[text_length] = '\0';

  ppb_width = OLED_StringWidth(ppb_text, OLED_FONT_6X8);
  OLED_ShowString((uint8_t)(OLED_WIDTH - ppb_width - 8U), 54U, ppb_text,
                  OLED_FONT_6X8);

  OLED_Refresh();
}

static void UI_ShowMessage(const char *value, const char *message)
{
  uint8_t value_width = OLED_StringWidth(value, OLED_FONT_24X48);
  uint8_t x = (value_width < OLED_WIDTH) ?
              (uint8_t)((OLED_WIDTH - value_width) / 2U) : 0U;

  OLED_ClearBuffer();
  OLED_ShowString(x, 2U, value, OLED_FONT_24X48);
  UI_ShowCenteredText(54U, message, OLED_FONT_6X8);
  OLED_Refresh();
}

static uint8_t FormatMgPerM3(uint16_t ppb, char *buffer)
{
  uint32_t thousandths = (((uint32_t)ppb * 1228U) + 500U) / 1000U;
  uint32_t whole = thousandths / 1000U;
  uint32_t fraction = thousandths % 1000U;
  uint8_t length = 0U;

  if (whole > 99U)
  {
    whole = 99U;
  }

  if (whole >= 10U)
  {
    buffer[length++] = (char)('0' + (whole / 10U));
    buffer[length++] = (char)('0' + (whole % 10U));
    buffer[length++] = '.';
    buffer[length++] = (char)('0' + (fraction / 100U));
    buffer[length++] = (char)('0' + ((fraction / 10U) % 10U));
  }
  else
  {
    buffer[length++] = (char)('0' + whole);
    buffer[length++] = '.';
    buffer[length++] = (char)('0' + (fraction / 100U));
    buffer[length++] = (char)('0' + ((fraction / 10U) % 10U));
    buffer[length++] = (char)('0' + (fraction % 10U));
  }
  buffer[length] = '\0';

  return length;
}

static uint8_t FormatU16(uint16_t value, char *buffer)
{
  char digits[5];
  uint8_t digit_count = 0U;
  uint8_t length = 0U;

  if (value == 0U)
  {
    buffer[0] = '0';
    buffer[1] = '\0';
    return 1U;
  }

  while ((value > 0U) && (digit_count < sizeof(digits)))
  {
    digits[digit_count++] = (char)('0' + (value % 10U));
    value /= 10U;
  }

  while (digit_count > 0U)
  {
    buffer[length++] = digits[--digit_count];
  }
  buffer[length] = '\0';

  return length;
}

static void LogString(const char *text)
{
  if (text != NULL)
  {
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)text,
                            (uint16_t)strlen(text), 100U);
  }
}

static void LogReading(uint16_t ppb)
{
  char text[40];
  char digits[8];
  const char *prefix = "[HCHO] ";
  const char *suffix = " ppb\r\n";
  uint8_t length = 0U;
  uint8_t count;

  while (*prefix != '\0')
  {
    text[length++] = *prefix++;
  }

  count = FormatU16(ppb, digits);
  memcpy(&text[length], digits, count);
  length = (uint8_t)(length + count);

  while (*suffix != '\0')
  {
    text[length++] = *suffix++;
  }

  text[length] = '\0';
  LogString(text);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
