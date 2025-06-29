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

#define ARM_MATH_CM4
#include "arm_math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_LED 54

#define MAX_BRIGHTNESS 255
#define NORMAL_BRIGHTNESS 128
#define USE_BRIGHTNESS 1

// for FFT
#define FFT_BUFFER_SIZE 2048
#define SAMPLE_RATE_HZ 14400.0f
#define UINT16_TO_FLOAT 0.00001525879f

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */

////////////////////////////////////////////////////////////////////////////////
/////////////////////////LED control////////////////////////////////////////////
uint8_t brightness_mode = NORMAL_BRIGHTNESS;
uint16_t LED_DURATION_MS = 100;
uint16_t amp = 0;
bool led_is_on = false;
uint32_t led_on_start = 0;

uint8_t LED_Data[MAX_LED][4];
uint8_t LED_Mod[MAX_LED][4];
int datasentflag = 0;
uint16_t pwmData[(24 * MAX_LED)+50];
uint16_t  effStep = 0;

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

////////////////////////////////////////////////////////////////////////////////
/////////////////////////Sound intensity control////////////////////////////////

int16_t middle_point_index = 32;
uint32_t middle_point = 0;

int amp_maxn = 1600; 	// Noise
int amp_maxq = 1000;	// Quiet


////////////////////////////////////////////////////////////////////////////////
/////////////////////////Frequency control (FFT)////////////////////////////////
uint16_t peakHz = 0;
arm_rfft_fast_instance_f32 fftHandler;
float fftBufIn[FFT_BUFFER_SIZE];
float fftBufOut[FFT_BUFFER_SIZE];
uint16_t fftIndex = 0;
uint8_t fftReady = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void record_sample_and_maybe_runFFT(uint16_t raw) {
    // Convert raw 12‐bit ADC (0..4095) to float in [-1,+1], after centering around middle_point

    float centered = ((float)(raw - (uint16_t)(middle_point))) * UINT16_TO_FLOAT;
    fftBufIn[fftIndex] = centered;
    fftIndex++;

    if (fftIndex >= FFT_BUFFER_SIZE) {
        // Run the RFFT in place: fftBufIn → fftBufOut
        arm_rfft_fast_f32(&fftHandler, fftBufIn, fftBufOut, 0);

        fftReady = 1;
        fftIndex = 0; // reset buffer index to record next block
    }
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

void Set_Brightness (int brightness)
{
#if USE_BRIGHTNESS
    if (brightness > MAX_BRIGHTNESS) brightness = MAX_BRIGHTNESS;
    float scale = brightness / (float)MAX_BRIGHTNESS;

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

// Helper function to map frequency to color temperature (warm to cool)
void frequency_to_color_temp(uint16_t freq, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (freq < 250) {
        // Low frequencies: Warm colors (Red to Orange)
        float ratio = (float)freq / 250.0f;
        *r = 255;
        *g = (uint8_t)(ratio * 165); // Orange component
        *b = 0;
    } else if (freq <= 2000) {
        // Mid frequencies: Yellow to Green
        float ratio = (float)(freq - 250) / 1750.0f;
        *r = (uint8_t)(255 * (1.0f - ratio)); // Fade out red
        *g = 255;
        *b = 0;
    } else {
        // High frequencies: Cool colors (Green to Blue to Violet)
        float ratio = (freq - 2000) / 18000.0f; // Up to 20kHz
        if (ratio > 1.0f) ratio = 1.0f;

        if (ratio < 0.5f) {
            // Green to Blue
            float t = ratio * 2.0f;
            *r = 0;
            *g = (uint8_t)(255 * (1.0f - t));
            *b = (uint8_t)(255 * t);
        } else {
            // Blue to Violet
            float t = (ratio - 0.5f) * 2.0f;
            *r = (uint8_t)(128 * t);
            *g = 0;
            *b = 255;
        }
    }
}

// Helper function to map frequency to full spectrum (Red to Violet)
void frequency_to_full_spectrum(uint16_t freq, uint8_t* r, uint8_t* g, uint8_t* b) {
    // Map frequency to hue (0-300 degrees)
    float hue = (float)freq / 7000.0f * 300.0f; // 0-7kHz mapped to 0-300°
    if (hue > 300.0f) hue = 300.0f;

    HSV_to_RGB(hue, 1.0f, 1.0f, r, g, b);

}

#define AMP_THRESHOLD 1000     // Amplitude threshold to detect a beat
#define FADE_DURATION_MS 500   // Fade duration in milliseconds
void effect_flash_fade_random_color(uint16_t amp, uint16_t peakHz, uint8_t brightness_mode) {
    static uint32_t last_flash_time = 0;  // Time of the last flash

    uint32_t current_time = HAL_GetTick();  // Current time in milliseconds

    // Beat detection: Check if amplitude exceeds threshold
    if (amp > AMP_THRESHOLD) {
        // Generate a random RGB color (0-255)
        uint8_t r = rand() % 256;
        uint8_t g = rand() % 256;
        uint8_t b = rand() % 256;

        // Set all LEDs to the new random color
        for (int i = 0; i < MAX_LED; i++) {
            Set_LED(i, r, g, b);
        }

        // Update the last flash time to start a new fade cycle
        last_flash_time = current_time;
    }

    // Calculate elapsed time since the last flash
    uint32_t elapsed_time = current_time - last_flash_time;

    // Calculate brightness based on elapsed time
    int brightness;
    if (elapsed_time >= FADE_DURATION_MS) {
        brightness = 0;  // Fully faded out
    } else {
        // Linear fade: brightness decreases from BRIGHTNESS_MODE to 0
        brightness = brightness_mode - (brightness_mode * elapsed_time) / FADE_DURATION_MS;
    }

    // Apply the calculated brightness to the LEDs
    Set_Brightness(brightness);

    // Update the LED strip
    WS2812_Send();
}

void effect_dynamic_vu_meter(uint16_t amplitude, uint16_t peak_freq, uint8_t brightness_mode) {
    // Calculate ratio and number of LEDs to light
    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;

    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
        return;
    }

    int total_leds_to_light = (int)(ratio * MAX_LED);
    if (total_leds_to_light < 1) total_leds_to_light = 1;

    // Get color based on frequency
    uint8_t r, g, b;
    frequency_to_full_spectrum(peak_freq, &r, &g, &b);

    // Clear all LEDs first
    Turn_off_all_at_once();

    // Light LEDs from center outward
    int center = MAX_LED / 2;
    int leds_per_side = total_leds_to_light / 2;

    // Light center LED if odd number
    if (total_leds_to_light % 2 == 1) {
        Set_LED(center, r, g, b);
    }

    // Light LEDs on both sides of center
    for (int i = 1; i <= leds_per_side; i++) {
        if (center - i >= 0) {
            Set_LED(center - i, r, g, b);
        }
        if (center + i < MAX_LED) {
            Set_LED(center + i, r, g, b);
        }
    }

    int brightness = 5 + (int)(ratio * (brightness_mode - 5));
    Set_Brightness(brightness);
    WS2812_Send();
}

void effect_spectrum_color_bands(uint16_t amplitude, uint16_t peak_freq, uint8_t brightness_mode) {
    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;

    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
        return;
    }

    // Define frequency bands and LED sections
    int low_start = 0;
    int low_end = MAX_LED / 3;
    int mid_start = low_end;
    int mid_end = (MAX_LED * 2) / 3;
    int high_start = mid_end;
    int high_end = MAX_LED;

    // Clear all LEDs
    Turn_off_all_at_once();

    // Determine which band the peak frequency falls into
    uint8_t r = 0, g = 0, b = 0;
    int start_led = 0, end_led = 0;

    if (peak_freq <= 250) {
        // Low frequency band - Red
        frequency_to_full_spectrum(peak_freq, &r, &g, &b);
        start_led = low_start;
        end_led = low_end;
    } else if (peak_freq <= 2000) {
        // Mid frequency band - Green
    	frequency_to_full_spectrum(peak_freq, &r, &g, &b);
        start_led = mid_start;
        end_led = mid_end;
    } else {
        // High frequency band - Blue
    	frequency_to_full_spectrum(peak_freq, &r, &g, &b);
        start_led = high_start;
        end_led = high_end;
    }

    // Light up the appropriate section
    for (int i = start_led; i < end_led; i++) {
        Set_LED(i, r, g, b);
    }

    int brightness = 5 + (int)(ratio * (brightness_mode - 5));
    Set_Brightness(brightness);
    WS2812_Send();
}

// Structure to hold pulse data
typedef struct {
    int position;           // Current position of pulse
    uint8_t r, g, b;       // Color of pulse
    uint8_t brightness;    // Brightness of pulse
    uint8_t length;        // Length of pulse in LEDs
    uint8_t active;        // Whether pulse is active
} Pulse;

#define MAX_PULSES 8
static Pulse pulses[MAX_PULSES];
static uint16_t last_amp = 0;
static uint32_t last_pulse_time = 0;

void effect_frequency_chase_gradient(uint16_t amplitude, uint16_t peak_freq, uint8_t brightness_mode) {
    const uint16_t BEAT_THRESHOLD = 800;  // Adjust based on your amp_maxn
    const uint32_t MIN_PULSE_INTERVAL = 100; // Minimum time between pulses (ms)

    uint32_t current_time = HAL_GetTick();

    // Beat detection: amplitude spike above threshold
    if (amplitude > BEAT_THRESHOLD &&
        amplitude > (last_amp + 200) &&
        (current_time - last_pulse_time) > MIN_PULSE_INTERVAL) {

        // Find an inactive pulse slot
        for (int i = 0; i < MAX_PULSES; i++) {
            if (!pulses[i].active) {
                // Initialize new pulse
                pulses[i].position = 0;
                frequency_to_full_spectrum(peak_freq, &pulses[i].r, &pulses[i].g, &pulses[i].b);

                float ratio = (float)amplitude / amp_maxn;
                if (ratio > 1.0f) ratio = 1.0f;

                pulses[i].brightness = (uint8_t)(ratio * 255);
                pulses[i].length = 3 + (uint8_t)(ratio * 8); // 3-11 LEDs long
                pulses[i].active = 1;

                last_pulse_time = current_time;
                break;
            }
        }
    }

    // Clear all LEDs
    Turn_off_all_at_once();

    // Update and draw all active pulses
    for (int i = 0; i < MAX_PULSES; i++) {
        if (pulses[i].active) {
            // Draw pulse with fade-out trail
            for (int j = 0; j < pulses[i].length; j++) {
                int led_pos = pulses[i].position - j;
                if (led_pos >= 0 && led_pos < MAX_LED) {
                    // Calculate fade factor for trail
                    float fade = 1.0f - ((float)j / pulses[i].length);
                    uint8_t r = (uint8_t)(pulses[i].r * fade);
                    uint8_t g = (uint8_t)(pulses[i].g * fade);
                    uint8_t b = (uint8_t)(pulses[i].b * fade);

                    // Blend with existing LED color (simple additive)
                    uint8_t existing_r = LED_Data[led_pos][2];
                    uint8_t existing_g = LED_Data[led_pos][1];
                    uint8_t existing_b = LED_Data[led_pos][3];

                    r = (r + existing_r > 255) ? 255 : r + existing_r;
                    g = (g + existing_g > 255) ? 255 : g + existing_g;
                    b = (b + existing_b > 255) ? 255 : b + existing_b;

                    Set_LED(led_pos, r, g, b);
                }
            }

            // Move pulse forward
            pulses[i].position++;

            // Deactivate pulse if it's moved off the strip
            if (pulses[i].position >= MAX_LED + pulses[i].length) {
                pulses[i].active = 0;
            }
        }
    }

    Set_Brightness(brightness_mode);
    WS2812_Send();

    last_amp = amplitude;
}

void effect_rainbow_roll(uint16_t amplitude, uint16_t peak_freq, uint8_t brightness_mode) {
    static float offset = 0.0f;

    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;

    // Cập nhật offset dựa vào tần số đỉnh
    offset += ((float)peak_freq / 100.0f);  // tốc độ cuộn
    if (offset >= 360.0f) offset -= 360.0f;

    for (int i = 0; i < MAX_LED; i++) {
        float hue = fmodf(offset + (i * (360.0f / MAX_LED)), 360.0f);
        uint8_t r, g, b;
        HSV_to_RGB(hue, 1.0f, ratio, &r, &g, &b);
        Set_LED(i, r, g, b);
    }

    int brightness = 5 + (int)(ratio * (brightness_mode - 5));
    Set_Brightness(brightness);
    WS2812_Send();
}

#define BASS_THRESHOLD 1200		// tăng để LED sáng hơn
#define BASS_FADE_MS 300		// tăng để LED tắt chậm hơn

void effect_bass_pulse_glow(uint16_t amplitude, uint16_t peak_freq, uint8_t brightness_mode) {
    static uint32_t last_bass_time = 0;
    static float hue = 0.0f;
    uint32_t now = HAL_GetTick();

    // Nếu là bass mạnh → cập nhật thời gian và màu sắc (hue)
    if (peak_freq <= 250 && amplitude > BASS_THRESHOLD) {
        last_bass_time = now;

        hue += 30.0f;  // Tăng hue để chuyển màu theo dải cầu vồng
        if (hue >= 360.0f) hue -= 360.0f;
    }

    // Tính độ sáng fade theo thời gian kể từ lần bass gần nhất
    uint32_t elapsed = now - last_bass_time;
    int brightness;

    if (elapsed >= BASS_FADE_MS) {
        brightness = 0;
    } else {
        brightness = brightness_mode - (brightness_mode * elapsed) / BASS_FADE_MS;
    }

    // Chuyển hue → RGB
    uint8_t r, g, b;
    HSV_to_RGB(hue, 1.0f, 1.0f, &r, &g, &b);  // Độ sáng sẽ được điều chỉnh sau bằng Set_Brightness()

    // Set toàn bộ LED thành màu đang có
    for (int i = 0; i < MAX_LED; i++) {
        Set_LED(i, r, g, b);
    }

    Set_Brightness(brightness);
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

  srand(HAL_GetTick());
  uint8_t mode_button_index = 0;
  uint8_t brightness_button_count = 0;

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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim2);
  HAL_ADC_Start_IT(&hadc1);
  arm_rfft_fast_init_f32(&fftHandler, FFT_BUFFER_SIZE);
  HAL_Delay(50); // delay de co thoi gian de lay sample offset_point

  float	peakVal	= 0.0f;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	/////////////////////////////////////////////////////////////////////////////////////////
	// FFT process
			  if (fftReady){
				  peakVal = 0.0f;
				  peakHz = 0.0f;

				  uint16_t halfBins = FFT_BUFFER_SIZE / 2;
				  for (uint16_t k = 1; k < halfBins; k++) {
					  float re = fftBufOut[2 * k];
					  float im = fftBufOut[2 * k + 1];
					  float mag = sqrtf(re * re + im * im);
					  if (mag > peakVal) {
						  peakVal = mag;
						  peakHz  = (uint16_t)(k  * SAMPLE_RATE_HZ / (float)(FFT_BUFFER_SIZE));
					  }
				  }

				  fftReady = 0;
			  }
	/////////////////////////////////////////////////////////////////////////////////////////
	// Effects
			  if(HAL_GPIO_ReadPin(MODE_BUTTON_GPIO_Port, MODE_BUTTON_Pin) == 0) {
				  while(HAL_GPIO_ReadPin(MODE_BUTTON_GPIO_Port, MODE_BUTTON_Pin) == 0) {}
				  mode_button_index = (mode_button_index + 1) % 6;
			  }

			  switch (mode_button_index) {
			  	  case 0: effect_flash_fade_random_color(amp, peakHz, brightness_mode); break;
			  	  case 1: effect_dynamic_vu_meter(amp, peakHz, brightness_mode); break;
			  	  case 2: effect_spectrum_color_bands(amp, peakHz, brightness_mode); break;
			  	  case 3: effect_frequency_chase_gradient(amp, peakHz, brightness_mode); break;
			  	  case 4: effect_rainbow_roll(amp, peakHz, brightness_mode); break;
			  	  case 5: effect_bass_pulse_glow(amp, peakHz, brightness_mode); break;
			  	  default: effect_flash_fade_random_color(amp, peakHz, brightness_mode); break;
			  }

			  if(HAL_GPIO_ReadPin(BRIGHTNESS_MODE_BUTTON_GPIO_Port, BRIGHTNESS_MODE_BUTTON_Pin) == 0) {
				  while(HAL_GPIO_ReadPin(BRIGHTNESS_MODE_BUTTON_GPIO_Port, BRIGHTNESS_MODE_BUTTON_Pin) == 0) {}
				  brightness_mode = (uint8_t)((float)MAX_BRIGHTNESS * ((25.0f * (float)brightness_button_count) / 100.0f));
				  brightness_button_count = (brightness_button_count + 1) % 5;
			  }
		  HAL_Delay(2);
	/////////////////////////////////////////////////////////////////////////////////////////

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
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
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
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pin : MODE_BUTTON_Pin */
  GPIO_InitStruct.Pin = MODE_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(MODE_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BRIGHTNESS_MODE_BUTTON_Pin */
  GPIO_InitStruct.Pin = BRIGHTNESS_MODE_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BRIGHTNESS_MODE_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
     GPIO_InitStruct.Pin  = GPIO_PIN_0;
     GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {

	if (hadc->Instance == ADC1)
	    {

			uint16_t raw = HAL_ADC_GetValue(&hadc1);

			// Tinh toan middle point tu 32 sample dau tien
			if ( middle_point_index > 0){
				middle_point += raw;
				middle_point_index--;
			}

			else if (middle_point_index == 0){
				middle_point/= 32;
				middle_point_index--;
			}

			else{
			// Ghi sample vao fftBufIn
				record_sample_and_maybe_runFFT(raw);
			}

	        amp = abs(raw - (uint16_t)middle_point);


	    }

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
