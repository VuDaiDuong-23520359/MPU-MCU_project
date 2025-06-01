/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "math.h"
#include "time.h"
#include "stdlib.h"
#include "stdbool.h"
#include <stdio.h>
#include "stm32f4xx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_LED 54
#define MAX_BRIGHTNESS 45
#define NORMAL_BRIGHTNESS 20
#define USE_BRIGHTNESS 1
#define PI 3.14159265


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */
uint16_t LED_DURATION_MS = 100;
uint16_t amp = 0;
bool led_is_on = false;
uint32_t led_on_start = 0;

uint8_t LED_Data[MAX_LED][4];
uint8_t LED_Mod[MAX_LED][4];
int datasentflag = 0;
uint16_t pwmData[(24 * MAX_LED)+50];
uint16_t  effStep = 0;

const int blockOn = 6;
const int blockOff = 8;
int patternLength = blockOn + blockOff;
uint16_t dynamic_threshold = 20;
uint8_t colors[7][3] = {
    {255, 0, 0},     // Red
    {255, 127, 0},   // Orange
    {255, 255, 0},   // Yellow
    {0, 255, 0},     // Green
    {0, 0, 255},     // Blue
    {75, 0, 130},    // Indigo
    {148, 0, 211}    // Violet
};

uint16_t amp_buffer[32];
int buffer_index = 0;
uint16_t middle_point = 0;

int amp_maxn = 1600; 	// Noise
int amp_maxq = 1000;	// Quiet

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */





void calculate_middle_point(){
	uint32_t sum = 0;
	for (int i = 0; i < 32; i++){
		  HAL_ADC_Start(&hadc1);
		  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
			  sum += HAL_ADC_GetValue(&hadc1);
		  HAL_ADC_Stop(&hadc1);
		  HAL_Delay(1);
	  }

	middle_point = (uint16_t)(sum / 32);
}


uint16_t get_amp()
{
	uint16_t temp = HAL_ADC_GetValue(&hadc1);
	return abs(temp - middle_point);
}



void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim1) {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
        datasentflag = 1;
    }
}


void Set_LED (int LEDnum, int Red, int Green, int Blue)
{
	LED_Data[LEDnum][0] = LEDnum;
	LED_Data[LEDnum][1] = Green;
	LED_Data[LEDnum][2] = Red;
	LED_Data[LEDnum][3] = Blue;
}

void Set_Brightness (int brightness)  // 0–NORMAL_BRIGHTNESS
{
#if USE_BRIGHTNESS
    if (brightness > NORMAL_BRIGHTNESS) brightness = NORMAL_BRIGHTNESS;
    float scale = brightness / (float)NORMAL_BRIGHTNESS;

    for (int i = 0; i < MAX_LED; i++)
    {
        // preserve the “LED number” byte
        LED_Mod[i][0] = LED_Data[i][0];
        // scale each color channel linearly
        LED_Mod[i][1] = (uint8_t)(LED_Data[i][1] * scale);
        LED_Mod[i][2] = (uint8_t)(LED_Data[i][2] * scale);
        LED_Mod[i][3] = (uint8_t)(LED_Data[i][3] * scale);
    }
#endif
}

void WS2812_Send (void)
{
	uint32_t indx=0;
	uint32_t color;


	for (int i= 0; i<MAX_LED; i++)
	{
#if USE_BRIGHTNESS
		color = ((LED_Mod[i][1]<<16) | (LED_Mod[i][2]<<8) | (LED_Mod[i][3]));
#else
		color = ((LED_Data[i][1]<<16) | (LED_Data[i][2]<<8) | (LED_Data[i][3]));
#endif

		for (int i=23; i>=0; i--)
		{
			if (color&(1<<i))
			{
				pwmData[indx] = 60;  // 2/3 of 90
			}

			else pwmData[indx] = 30;  // 1/3 of 90

			indx++;
		}

	}

	for (int i=0; i<50; i++)
	{
		pwmData[indx] = 0;
		indx++;
	}

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);
	while (!datasentflag){};
	datasentflag = 0;
}

void HSV_to_RGB(float h, float s, float v, uint8_t* r, uint8_t* g, uint8_t* b) {
    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    float r_, g_, b_;

    switch (i) {
        case 0: r_ = v; g_ = t; b_ = p; break;
        case 1: r_ = q; g_ = v; b_ = p; break;
        case 2: r_ = p; g_ = v; b_ = t; break;
        case 3: r_ = p; g_ = q; b_ = v; break;
        case 4: r_ = t; g_ = p; b_ = v; break;
        default: r_ = v; g_ = p; b_ = q; break;
    }

    *r = (uint8_t)(r_ * 255);
    *g = (uint8_t)(g_ * 255);
    *b = (uint8_t)(b_ * 255);
}

//effect
void Set_LEDs_color_at_once(int start, int end, int step, int r, int g, int b){
	for (int pos = start; pos < end; pos += step){
		Set_LED(pos, r, g, b);		// purple
	}
}

void Turn_on_all_at_once(int r, int g, int b, int to_led){
	Set_LEDs_color_at_once(0, to_led, 1, r, g, b);
	Set_Brightness(NORMAL_BRIGHTNESS);
	WS2812_Send();
}



void Turn_off_all_at_once(void){
	Set_LEDs_color_at_once(0, MAX_LED, 1, 0, 0, 0);
	Set_Brightness(NORMAL_BRIGHTNESS);
	WS2812_Send();
}

void get_rainbow_color(uint16_t index, uint16_t effStep, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    // Compute a “phase” value in [0, 59], matching the original rainbow_effect logic
    int16_t temp = (int16_t)(effStep - index * 1.2f);
    int16_t mod  = temp % 60;            // may be negative or positive
    if (mod < 0) mod += 60;              // ensure 0 ≤ mod < 60
    uint16_t ind = 60 - mod;             // ind in [1, 60]
    ind = ind % 60;                      // now in [0, 59]

    // Determine which third of the 60-step “wheel” we’re in
    uint16_t segment = (ind % 60) / 20;  // 0 → red→green, 1 → green→blue, 2 → blue→red

    float factor1, factor2;
    uint8_t rr = 0, gg = 0, bb = 0;

    switch (segment) {
        case 0:
            // Transition red → green
            // factor1 goes from 1 → 0 over 20 steps; factor2 goes from 0 → 1
            factor1 = 1.0f - ((float)(ind % 20) / 20.0f);
            factor2 = ((float)(ind % 20) / 20.0f);
            rr = (uint8_t)(255.0f * factor1);
            gg = (uint8_t)(255.0f * factor2);
            bb = 0;
            break;

        case 1:
            // Transition green → blue
            factor1 = 1.0f - ((float)((ind % 60) - 20) / 20.0f);
            factor2 = ((float)((ind % 60) - 20) / 20.0f);
            rr = 0;
            gg = (uint8_t)(255.0f * factor1);
            bb = (uint8_t)(255.0f * factor2);
            break;

        case 2:
            // Transition blue → red
            factor1 = 1.0f - ((float)((ind % 60) - 40) / 20.0f);
            factor2 = ((float)((ind % 60) - 40) / 20.0f);
            rr = (uint8_t)(255.0f * factor2);
            gg = 0;
            bb = (uint8_t)(255.0f * factor1);
            break;
    }

    // Store results
    *red = rr;
    *green = gg;
    *blue = bb;
}

void effect_sound_color() {
    float ratio = (float)amp / amp_maxn;
    if (ratio > 1.0) ratio = 1.0;
    if (ratio <= 0.05f) {
    	Turn_off_all_at_once();
    	WS2812_Send();
    	return;
    }

    int brightness = 5 + (int)(ratio * (NORMAL_BRIGHTNESS - 5));
    Set_Brightness(brightness);

    uint8_t r, g, b;

    if (ratio < 0.5f) {
        float t = ratio / 0.5f;
        r = (uint8_t)(0 + t * (148 - 0));
        g = 0;
        b = (uint8_t)(255 + t * (211 - 255));
    } else {
        float t = (ratio - 0.5f) / 0.5f;
        if (t < 0.5f) {
            float t2 = t / 0.5f;
            r = 255;
            g = (uint8_t)(0 + t2 * (127 - 0));
            b = 0;
        } else {
            float t2 = (t - 0.5f) / 0.5f;
            r = 255;
            g = (uint8_t)(127 + t2 * (255 - 127));
            b = 0;
        }
    }

    for (int i = 0; i < MAX_LED; i++) {
        Set_LED(i, r, g, b);
    }
    WS2812_Send();
}

void ripple_effect(uint16_t amplitude) {
    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;

    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
        WS2812_Send();
        return;
    }

    int center = MAX_LED / 2;
    int spread = 1 + (int)(ratio * (MAX_LED / 2));

    for (int i = 0; i < MAX_LED; i++) {
        int dist = abs(i - center);
        float brightness = 1.0f - ((float)dist / spread);
        if (brightness < 0.0f) brightness = 0.0f;

        // Get rainbow color based on position
        uint8_t r, g, b;
        get_rainbow_color(i, effStep, &r, &g, &b);

        // Apply brightness modulation to color
        Set_LED(i, (uint8_t)(r * brightness), (uint8_t)(g * brightness), (uint8_t)(b * brightness));
    }

    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();

    effStep = (effStep + 1) % 60;
}


void sound_bar_hue_gradient(uint16_t amplitude) {
    static int current_led_count = 1;
    int target_led_count;

    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;

    target_led_count = (int)(ratio * MAX_LED);
    if (target_led_count < 1) target_led_count = 1;

    if (amplitude < 100 && current_led_count > 1) {
        current_led_count--;
    } else if (target_led_count > current_led_count) {
        current_led_count++;
    }

    Turn_off_all_at_once();

    for (int i = 0; i < current_led_count; i++) {
        float t = (float)i / (current_led_count - 1);
        float hue = 270.0f * (1.0f - t); // tím (270°) → đỏ (0°)

        uint8_t r, g, b;
        HSV_to_RGB(hue, 1.0f, 1.0f, &r, &g, &b);
        Set_LED(i, r, g, b);
    }

    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

void effect_random_one_in_six_leds_by_sound(uint16_t amplitude) {
    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;

    // Ignore low amplitude (silence)
    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
        WS2812_Send();
        return;
    }

    Turn_off_all_at_once();

    const int group_size = 6; //per n Led will have 1 random led lighted
    for (int start = 0; start < MAX_LED; start += group_size) {
        int rand_index = rand() % group_size;
        int led_index = start + rand_index;
        if (led_index >= MAX_LED) break;

        // Optional: brighter when louder
        uint8_t brightness = (uint8_t)(ratio * 255);
        uint8_t r = brightness;
        uint8_t g = rand() % brightness;
        uint8_t b = rand() % brightness;

        Set_LED(led_index, r, g, b);
    }

    Set_Brightness(5 + (int)(ratio * (NORMAL_BRIGHTNESS - 5)));
    WS2812_Send();
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
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DUNG CO DUNG VAO
  calculate_middle_point();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_ADC_Start(&hadc1);
	  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
		  amp = get_amp();
	  HAL_ADC_Stop(&hadc1);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
      if (!led_is_on && amp > 700)
      {
          led_on_start = HAL_GetTick();
          led_is_on    = true;
      }

      /* While led_is_on, run effect; after 50 ms, stop --- */
      if (led_is_on)
      {
          sound_bar_hue_gradient(amp);

          if ((HAL_GetTick() - led_on_start) >= 50)
          {
              Turn_off_all_at_once();
              WS2812_Send();
              led_is_on = false;
          }
      }
      else
      {
          Turn_off_all_at_once();
          WS2812_Send();
      }

	  HAL_Delay(1);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 90 - 1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
     GPIO_InitStruct.Pin  = GPIO_PIN_0;
     GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
//    if (hadc == &hadc1) {
//        uint16_t amp = process_buffer(adc_buf, ADC_BUF_LEN/2);
//        uint8_t br  = map_amplitude_to_brightness(amp, dynamic_threshold);
//        Set_Brightness(br);
//        WS2812_Send();
//    }
//}
//
//// Called when second half of adc_buf is filled by DMA (another 32 samples)
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//		amp = HAL_ADC_GetValue(hadc);
//
//
//}
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

#ifdef  USE_FULL_ASSERT
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
