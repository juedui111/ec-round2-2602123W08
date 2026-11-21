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
#include "tim.h"
#include "usart.h"
#include "usb.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "servo.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13
#define KEY_PORT        GPIOB
#define KEY_PIN         GPIO_PIN_12

#define LED_MODE_BLINK  0U
#define LED_MODE_OFF    1U
#define LED_MODE_ON     2U
#define LED_MODE_COUNT  3U

#define KEY_DEBOUNCE_MS 20U
#define LED_BLINK_MS    500U

/* T1 单轴扫动：1 表示走满 500-2500us，0 表示只用中段以免未标定时撞限位 */
#define PWM_TEST_FULL_RANGE 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t s_led_mode = LED_MODE_BLINK;
static uint32_t s_led_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void app_self_check(void);
static const char *app_reset_cause(void);
static uint8_t app_key_take_press(void);
static void app_led_update(void);
static void app_pwm_test(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* printf 重定向：newlib 的 _write 是 weak 符号，这里覆盖它 */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
  return len;
}

static const char *app_reset_cause(void)
{
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))  return "POR";
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))  return "PIN";
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))  return "SOFTWARE";
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) return "IWDG";
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) return "WWDG";
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)) return "LPWR";
  return "UNKNOWN";
}

static void app_self_check(void)
{
  printf("\r\n================================\r\n");
  printf(" ec-round2-2602123W08  T0 自检\r\n");
  printf("================================\r\n");
  printf("reset   : %s\r\n", app_reset_cause());
  printf("SYSCLK  : %lu MHz\r\n", (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000U));
  printf("HCLK    : %lu MHz\r\n", (unsigned long)(HAL_RCC_GetHCLKFreq() / 1000000U));
  printf("PCLK1   : %lu MHz\r\n", (unsigned long)(HAL_RCC_GetPCLK1Freq() / 1000000U));
  printf("PCLK2   : %lu MHz\r\n", (unsigned long)(HAL_RCC_GetPCLK2Freq() / 1000000U));
  printf("TIM2    : PSC=%lu ARR=%lu\r\n",
         (unsigned long)htim2.Init.Prescaler,
         (unsigned long)htim2.Init.Period);
  printf("UART1   : %lu bps\r\n", (unsigned long)huart1.Init.BaudRate);
  printf("LED     : PC13, 1Hz blink\r\n");
  printf("KEY     : PB12, short press to switch LED mode\r\n");
  printf("================================\r\n");
}

/* 按键消抖：检测到一次稳定的按下动作返回 1 */
static uint8_t app_key_take_press(void)
{
  static uint8_t s_level = 0;
  static uint8_t s_stable = 0;
  static uint32_t s_change_tick = 0;

  uint8_t level = (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
  uint32_t now = HAL_GetTick();

  if (level != s_level)
  {
    s_level = level;
    s_change_tick = now;
    return 0;
  }

  if ((s_stable != s_level) && ((now - s_change_tick) >= KEY_DEBOUNCE_MS))
  {
    s_stable = s_level;
    return s_stable;
  }

  return 0;
}

static void app_led_update(void)
{
  uint32_t now = HAL_GetTick();

  switch (s_led_mode)
  {
  case LED_MODE_BLINK:
    if ((now - s_led_tick) >= LED_BLINK_MS)
    {
      s_led_tick = now;
      HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
    break;

  case LED_MODE_OFF:
    /* PC13 低电平点亮，所以高电平是灭 */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    break;

  case LED_MODE_ON:
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    break;

  default:
    s_led_mode = LED_MODE_BLINK;
    break;
  }
}

/* T1：CH1 每隔 1.5s 换一个脉宽，方便示波器或万用表读数 */
static void app_pwm_test(void)
{
  static const uint16_t seq_full[] = { 1500U, 500U, 1000U, 1500U, 2000U, 2500U };
  static const uint16_t seq_safe[] = { 1500U, 1000U, 1250U, 1500U, 1750U, 2000U };
  static uint32_t t_last = 0;
  static uint8_t  idx = 0;

  const uint16_t *seq = (PWM_TEST_FULL_RANGE != 0) ? seq_full : seq_safe;
  const uint8_t   len = (uint8_t)(sizeof(seq_full) / sizeof(seq_full[0]));
  uint32_t now = HAL_GetTick();

  if ((now - t_last) < 1500U)
  {
    return;
  }

  t_last = now;

  servo_write_us(0U, seq[idx]);
  printf("[PWM] CH1 = %u us\r\n", (unsigned)servo_get_us(0U));

  idx = (uint8_t)((idx + 1U) % len);
}
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
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USB_PCD_Init();
  /* USER CODE BEGIN 2 */
  servo_init();
  app_self_check();
  s_led_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (app_key_take_press())
    {
      s_led_mode = (s_led_mode + 1U) % LED_MODE_COUNT;
      if (s_led_mode == LED_MODE_BLINK)
      {
        s_led_tick = HAL_GetTick();
      }
      printf("[KEY] LED mode -> %lu\r\n", (unsigned long)s_led_mode);
    }

    app_led_update();
    app_pwm_test();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
