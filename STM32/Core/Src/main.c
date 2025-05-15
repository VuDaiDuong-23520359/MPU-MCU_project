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
TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */

uint8_t LED_Data[MAX_LED][4];
uint8_t LED_Mod[MAX_LED][4];
int datasentflag = 0;
uint16_t pwmData[(24 * MAX_LED)+50];
uint16_t  effStep = 0;

const int blockOn = 6;
const int blockOff = 8;
int patternLength = blockOn + blockOff;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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

void Set_Brightness (int brightness)  // 0-45
{
#if USE_BRIGHTNESS

	if (brightness > 45) brightness = 45;
	for (int i=0; i<MAX_LED; i++)
	{
		LED_Mod[i][0] = LED_Data[i][0];
		for (int j=1; j<4; j++)
		{
			float angle = 90-brightness;  // in degrees
			angle = angle*PI / 180;  // in rad
			LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
		}
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

//effect
void Set_LEDs_color_at_once(int start, int end, int step, int r, int g, int b){
	for (int pos = start; pos < end; pos += step){
		Set_LED(pos, r, g, b);		// purple
	}
}

uint8_t rainbow_effect() {
    float factor1, factor2;
    uint16_t ind;

    for (uint16_t j = 0; j < MAX_LED; j++) {
        ind = 60 - (int16_t)(effStep - j * 1.2) % 60;
        switch ((int)((ind % 60) / 20)) {
            case 0:
                factor1 = 1.0 - ((float)(ind % 60 - 0 * 20) / 20);
                factor2 = (float)((int)(ind - 0) % 60) / 20;
                Set_LED(j, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2);
                break;
            case 1:
                factor1 = 1.0 - ((float)(ind % 60 - 1 * 20) / 20);
                factor2 = (float)((int)(ind - 20) % 60) / 20;
                Set_LED(j, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2);
                break;
            case 2:
                factor1 = 1.0 - ((float)(ind % 60 - 2 * 20) / 20);
                factor2 = (float)((int)(ind - 40) % 60) / 20;
                Set_LED(j, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2);
                break;
        }
    }

    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();

    effStep++;
    if (effStep >= 60) {
        effStep = 0;
        return 0x03;
    }
    return 0x01;
}


void On_off_single_led_ltor(void){
	for (int i = 0; i < MAX_LED; i++)
	{
		  Set_LED(i, 255, 0, 0);
		  Set_Brightness(NORMAL_BRIGHTNESS);
		  WS2812_Send();
		  HAL_Delay (30);
	}
	for (int i = MAX_LED - 1; i >= 0; i--)
	{
		  Set_LED(i, 0, 0, 0);
		  Set_Brightness(NORMAL_BRIGHTNESS);
		  WS2812_Send();
		  HAL_Delay(30);
	}
}

void Brightness_Color_Fade_Effect(void)
{
    int brightness;

    // Fade in
    for (brightness = 0; brightness <= NORMAL_BRIGHTNESS; brightness++)
    {
        uint8_t red = (brightness <= 15) ? (brightness * 17) : (brightness <= 30 ? (45 - brightness) * 17 : 0);
        uint8_t green = (brightness <= 15) ? 0 : (brightness <= 30 ? (brightness - 15) * 17 : (45 - brightness) * 17);
        uint8_t blue = (brightness >= 30) ? (brightness - 30) * 17 : 0;

        for (int i = 0; i < MAX_LED; i++)
            Set_LED(i, red, green, blue);

        Set_Brightness(brightness);
        WS2812_Send();
        HAL_Delay(30);
    }

    // Fade out
    for (brightness = NORMAL_BRIGHTNESS; brightness >= 0; brightness--)
    {
        uint8_t red = (brightness <= 15) ? (brightness * 17) : (brightness <= 30 ? (45 - brightness) * 17 : 0);
        uint8_t green = (brightness <= 15) ? 0 : (brightness <= 30 ? (brightness - 15) * 17 : (45 - brightness) * 17);
        uint8_t blue = (brightness >= 30) ? (brightness - 30) * 17 : 0;

        for (int i = 0; i < MAX_LED; i++)
            Set_LED(i, red, green, blue);

        Set_Brightness(brightness);
        WS2812_Send();
        HAL_Delay(30);
    }
}

void On_off_From2Side(void) {
    int half = MAX_LED / 2;

    // Turn on from both sides
    for (int i = 0; i < half; i++) {
        Set_LED(i, 255, 0, 0);                 // Left half: Red
        Set_LED(MAX_LED - 1 - i, 0, 0, 255);   // Right half: Blue
        Set_Brightness(NORMAL_BRIGHTNESS);
        WS2812_Send();
        HAL_Delay(30);
    }

    // Swap colors for both sides
    for (int i = half - 1; i >= 0; i--) {
        Set_LED(i, 0, 0, 255);                 // Left: Blue
        Set_LED(MAX_LED - i + 1, 255, 0, 0);   // Right: Red
        Set_Brightness(NORMAL_BRIGHTNESS);
        WS2812_Send();
        HAL_Delay(30);
    }

    // Turn off from both sides
    for (int i = 0; i < half; i++) {
        Set_LED(i, 0, 0, 0);
        Set_LED(MAX_LED - 1 - i, 0, 0, 0);
        Set_Brightness(NORMAL_BRIGHTNESS);
        WS2812_Send();
        HAL_Delay(30);
    }
}

void Blink_all(void){
	Set_LEDs_color_at_once(0, MAX_LED, 1, 180, 0, 180);
	Set_Brightness(NORMAL_BRIGHTNESS);
	WS2812_Send();
	HAL_Delay(30);

	Set_LEDs_color_at_once(0, MAX_LED, 1, 0, 0, 0);
	Set_Brightness(NORMAL_BRIGHTNESS);
	WS2812_Send();
	HAL_Delay(30);
}

void On_Off_Groups_Parallel(void) {
    for (int i = 0; i <= 5; i++){
    	Set_LEDs_color_at_once(i, MAX_LED, 6, 180, 0, 180);
    	Set_Brightness(NORMAL_BRIGHTNESS);
    	WS2812_Send();
    	HAL_Delay(50);
    }
    for (int i = 0; i <= 5; i++){
    	Set_LEDs_color_at_once(i, MAX_LED, 6, 0, 0, 0);
    	Set_Brightness(NORMAL_BRIGHTNESS);
    	WS2812_Send();
       	HAL_Delay(50);
    }
}

void r7LEDs_rainbow(void) {
    uint8_t colors[7][3] = {
        {255, 0, 0},     // Red
        {255, 127, 0},   // Orange
        {255, 255, 0},   // Yellow
        {0, 255, 0},     // Green
        {0, 0, 255},     // Blue
        {75, 0, 130},    // Indigo
        {148, 0, 211}    // Violet
    };

    static int head = 0;  // Red LED index (moves forward every call)

    // --- Tail cleanup ---
    // Find the LED that just moved out of the rainbow window
    int cleanup_idx = (head - 7 + MAX_LED) % MAX_LED;
    Set_LED(cleanup_idx, 0, 0, 0);

    // --- Draw current rainbow ---
    for (int j = 0; j < 7; j++) {
        int pos = (head - j + MAX_LED) % MAX_LED;
        Set_LED(pos, colors[j][0], colors[j][1], colors[j][2]);
    }

    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
    HAL_Delay(100);

    head = (head + 1) % MAX_LED;
}

void r7LEDs_rainbow_reverse(void) {
    static uint8_t colors[7][3] = {
        {255, 0, 0},     // Red
        {255, 127, 0},   // Orange
        {255, 255, 0},   // Yellow
        {0, 255, 0},     // Green
        {0, 0, 255},     // Blue
        {75, 0, 130},    // Indigo
        {148, 0, 211}    // Violet
    };

    static int head = 0;
    static int direction = 1;     // 1: forward, -1: backward
    static int waiting = 0;       // flag: waiting for tail to finish
    static int trail_index = 0;   // index for clearing trail

    // If in waiting mode, only clear trail one step at a time
    if (waiting) {
        int idx;
        if (direction == 1) {
            // Forward: tail was last 6 LEDs before head
            idx = head - 7 + trail_index;
        } else {
            // Backward: tail was next 6 LEDs after head
            idx = head + 7 - trail_index;
        }

        if (idx >= 0 && idx < MAX_LED) {
            Set_LED(idx, 0, 0, 0);  // Turn off tail
        }

        trail_index++;
        WS2812_Send();
        HAL_Delay(20);

        if (trail_index >= 7) {
            // Done clearing trail
            waiting = 0;
            trail_index = 0;
            direction *= -1;
            head += direction;  // Start next direction
        }
        return;
    }

    // Clear previous tail
    int clear_idx = (direction == 1) ? head - 7 : head + 7;
    if (clear_idx >= 0 && clear_idx < MAX_LED) {
        Set_LED(clear_idx, 0, 0, 0);
    }

    // Set current rainbow
    for (int j = 0; j < 7; j++) {
        int idx = head - j * direction;
        if (idx >= 0 && idx < MAX_LED) {
            Set_LED(idx, colors[j][0], colors[j][1], colors[j][2]);
        }
    }

    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
    HAL_Delay(100);

    // Move red head
    head += direction;

    // Start waiting when red reaches out of bounds
    if ((direction == 1 && head >= MAX_LED + 7) ||
        (direction == -1 && head < -7)) {
        waiting = 1;
        trail_index = 0;
        head -= direction;  // stay at edge while waiting
    }
}

void Blink_Groups_Parallel(void) {
    for (int i = 0; i <= 5; i++){
    	Set_LEDs_color_at_once(i, MAX_LED, 6, 180, 0, 180);
    	Set_Brightness(NORMAL_BRIGHTNESS);
    	WS2812_Send();
    	HAL_Delay(50);

    	Set_LEDs_color_at_once(i, MAX_LED, 6, 0, 0, 0);
    	Set_Brightness(NORMAL_BRIGHTNESS);
    	WS2812_Send();
    }
}

void Blink_Random_per6LEDs(void){
	const int group_count = 9;
	const int group_size = 6;

	Set_LEDs_color_at_once(0, MAX_LED, 1, 0, 0, 0);
	Set_Brightness(NORMAL_BRIGHTNESS);
	WS2812_Send();

    for (int i = 0; i < group_count; i++) {
        int start = i * group_size;
        int random_value = start + (rand() % group_size);
        Set_LED(random_value, 180, 0, 180);
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
    HAL_Delay(30);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  srand(time(NULL));
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
  /* USER CODE BEGIN 2 */


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	 r7LEDs_rainbow_reverse();
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
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
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
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
