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
  * This software is licensed as‐is. Do not redistribute.
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

// We want to run a simple RFFT on every 2048 audio samples:
#define ARM_MATH_CM4
#include "arm_math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* (no changes here) */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_LED 54

#define MAX_BRIGHTNESS 45
#define NORMAL_BRIGHTNESS 20
#define USE_BRIGHTNESS 1

#define PI 3.14159265f

// for FFT
#define FFT_BUFFER_SIZE    2048
#define AUDIO_BUFFER_SIZE   256   // not used for DMA here, but kept for reference
#define SAMPLE_RATE_HZ   48828.0f  // sampling rate
#define INT16_TO_FLOAT   0.00003051757f // to convert raw ADC reading (0..4095) to ±1.0 range
#define FLOAT_TO_INT16   32768.0f       // to convert back to int16 (if needed)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* (no changes here) */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */
// Existing amplitude‐trigger logic
uint16_t LED_DURATION_MS = 100;
uint16_t amp = 0;
bool     led_is_on = false;
uint32_t led_on_start = 0;

// WS2812 driver buffers
uint8_t  LED_Data[MAX_LED][4];
uint8_t  LED_Mod[MAX_LED][4];
int      datasentflag = 0;
uint16_t pwmData[(24 * MAX_LED) + 50];
uint16_t effStep = 0;

// dynamic threshold variables (unused in new code but left for reference)
uint16_t dynamic_threshold = 20;
uint8_t  colors[7][3] = {
    {255,   0,   0},   // Red
    {255, 127,   0},   // Orange
    {255, 255,   0},   // Yellow
    {  0, 255,   0},   // Green
    {  0,   0, 255},   // Blue
    { 75,   0, 130},   // Indigo
    {148,   0, 211}    // Violet
};

uint16_t amp_buffer[32];
int      buffer_index = 0;
uint16_t middle_point = 0;

// Noise / quiet calibration (you may want to adjust these)
int amp_maxn = 1600;
int amp_maxq = 1000;

// *** FFT‐related variables ***
arm_rfft_fast_instance_f32 fftHandler;
float fftBufIn[FFT_BUFFER_SIZE];
float fftBufOut[FFT_BUFFER_SIZE];
uint16_t fftIndex = 0;         // how many samples have been fed so far
float peakVal = 0.0f;          // magnitude of the peak bin
uint16_t peakHz = 0;           // frequency (in Hz) of the current peak
uint8_t  fftReady = 0;         // flag: set to 1 whenever a full FFT completes

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
/* (no additional prototypes here) */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Calibrate the “middle” ADC value by averaging 32 raw readings.
 *         This helps remove DC offset from a mic or line‐in source.
 */
void calculate_middle_point(void) {
    uint32_t sum = 0;
    for (int i = 0; i < 32; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            sum += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
        HAL_Delay(1);
    }
    middle_point = (uint16_t)(sum / 32);
}

/**
 * @brief  Return |(raw ADC) - (middle_point)|, giving a crude amplitude reading.
 */
uint16_t get_amp(void) {
    uint16_t raw = HAL_ADC_GetValue(&hadc1);
    return (uint16_t)abs((int)raw - (int)middle_point);
}

/**
 * @brief  Callback from TIM1's DMA completion: signaled when all pwmData[] frames
 *         have been sent to WS2812.
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim1) {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
        datasentflag = 1;
    }
}

/**
 * @brief  Prepare one LED's data (GRB order) into LED_Data[][].
 * @param  LEDnum: 0..MAX_LED-1
 * @param  Red, Green, Blue: 0..255
 */
void Set_LED(int LEDnum, int Red, int Green, int Blue) {
    LED_Data[LEDnum][0] = LEDnum;    // index byte (ignored by WS2812 logic but kept for safety)
    LED_Data[LEDnum][1] = Green;     // GRB order
    LED_Data[LEDnum][2] = Red;
    LED_Data[LEDnum][3] = Blue;
}

/**
 * @brief  Scale all LEDs' color channels by (brightness / NORMAL_BRIGHTNESS).
 *         Stores scaled values in LED_Mod[][]. If USE_BRIGHTNESS==0, bypasses.
 */
void Set_Brightness(int brightness) {
#if USE_BRIGHTNESS
    if (brightness > NORMAL_BRIGHTNESS) brightness = NORMAL_BRIGHTNESS;
    float scale = brightness / (float)NORMAL_BRIGHTNESS;
    for (int i = 0; i < MAX_LED; i++) {
        LED_Mod[i][0] = LED_Data[i][0];
        LED_Mod[i][1] = (uint8_t)(LED_Data[i][1] * scale);
        LED_Mod[i][2] = (uint8_t)(LED_Data[i][2] * scale);
        LED_Mod[i][3] = (uint8_t)(LED_Data[i][3] * scale);
    }
#endif
}

/**
 * @brief  Copy either LED_Mod[][] (if USE_BRIGHTNESS) or LED_Data[][] to
 *         pwmData[], then start TIM1 PWM‐DMA to send the WS2812 bit pulses.
 */
void WS2812_Send(void) {
    uint32_t indx = 0;
    uint32_t color;
    for (int i = 0; i < MAX_LED; i++) {
#if USE_BRIGHTNESS
        color = ((uint32_t)LED_Mod[i][1] << 16) | ((uint32_t)LED_Mod[i][2] << 8) | ((uint32_t)LED_Mod[i][3]);
#else
        color = ((uint32_t)LED_Data[i][1] << 16) | ((uint32_t)LED_Data[i][2] << 8) | ((uint32_t)LED_Data[i][3]);
#endif
        for (int bit = 23; bit >= 0; bit--) {
            if (color & (1UL << bit)) {
                pwmData[indx++] = 60;  // “1” = high duty‐cycle (2/3 of 90)
            } else {
                pwmData[indx++] = 30;  // “0” = low duty‐cycle (1/3 of 90)
            }
        }
    }
    // after all pixels, send ~50 “zeros” (reset pulse)
    for (int i = 0; i < 50; i++) {
        pwmData[indx++] = 0;
    }
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);
    while (!datasentflag) { /* wait until done */ }
    datasentflag = 0;
}

/**
 * @brief  Convert HSV (h in [0,360)) to 8‐bit RGB. Used by some effects.
 */
void HSV_to_RGB(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
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
    *r = (uint8_t)(r_ * 255.0f);
    *g = (uint8_t)(g_ * 255.0f);
    *b = (uint8_t)(b_ * 255.0f);
}

/**
 * @brief  Simple helper: fill LEDs [start..end) with (r,g,b) at step interval.
 */
void Set_LEDs_color_at_once(int start, int end, int step, int r, int g, int b) {
    for (int pos = start; pos < end; pos += step) {
        Set_LED(pos, r, g, b);
    }
}

/**
 * @brief  Turn entire strip to (r,g,b), then send.
 */
void Turn_on_all_at_once(int r, int g, int b, int to_led) {
    Set_LEDs_color_at_once(0, to_led, 1, r, g, b);
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  Turn all LEDs off, then send.
 */
void Turn_off_all_at_once(void) {
    Set_LEDs_color_at_once(0, MAX_LED, 1, 0, 0, 0);
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  Get a pseudo‐rainbow color for a given LED index + effStep.
 *         Returns r,g,b [0..255].
 */
void get_rainbow_color(uint16_t index, uint16_t effStep, uint8_t *red, uint8_t *green, uint8_t *blue) {
    /* Map index+effStep onto a 0..59 “wheel” (three segments of 20 steps each). */
    int16_t temp = (int16_t)(effStep - index * 1.2f);
    int16_t mod  = temp % 60;
    if (mod < 0) mod += 60;
    uint16_t ind = (60 - mod) % 60; // 0..59
    uint16_t segment = (ind / 20);  // 0,1,2
    float factor1, factor2;
    uint8_t rr = 0, gg = 0, bb = 0;
    switch (segment) {
        case 0: // Red→Green
            factor1 = 1.0f - ((ind % 20) / 20.0f);
            factor2 = ((ind % 20) / 20.0f);
            rr = (uint8_t)(255.0f * factor1);
            gg = (uint8_t)(255.0f * factor2);
            bb = 0;
            break;
        case 1: // Green→Blue
            factor1 = 1.0f - (((ind % 60) - 20) / 20.0f);
            factor2 = (((ind % 60) - 20) / 20.0f);
            rr = 0;
            gg = (uint8_t)(255.0f * factor1);
            bb = (uint8_t)(255.0f * factor2);
            break;
        case 2: // Blue→Red
            factor1 = 1.0f - (((ind % 60) - 40) / 20.0f);
            factor2 = (((ind % 60) - 40) / 20.0f);
            rr = (uint8_t)(255.0f * factor2);
            gg = 0;
            bb = (uint8_t)(255.0f * factor1);
            break;
    }
    *red   = rr;
    *green = gg;
    *blue  = bb;
}

/**
 * @brief  Existing effect: sound‐based color fade (red→violet→etc) based on amp.
 */
void effect_sound_color(void) {
    float ratio = (float)amp / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
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

/**
 * @brief  Existing ripple effect based on amplitude; rainbow colors.
 */
void ripple_effect(uint16_t amplitude) {
    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
        return;
    }

    int center = MAX_LED / 2;
    int spread = 1 + (int)(ratio * (MAX_LED / 2));
    for (int i = 0; i < MAX_LED; i++) {
        int dist = abs(i - center);
        float brightness = 1.0f - ((float)dist / spread);
        if (brightness < 0.0f) brightness = 0.0f;
        uint8_t r, g, b;
        get_rainbow_color(i, effStep, &r, &g, &b);
        Set_LED(i,
                (uint8_t)(r * brightness),
                (uint8_t)(g * brightness),
                (uint8_t)(b * brightness));
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
    effStep = (effStep + 1) % 60;
}

/**
 * @brief  Existing “sound‐bar” effect with hue gradient from purple→red.
 */
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
        float hue = 270.0f * (1.0f - t); // 270° (purple) → 0° (red)
        uint8_t r, g, b;
        HSV_to_RGB(hue, 1.0f, 1.0f, &r, &g, &b);
        Set_LED(i, r, g, b);
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  Existing “random one in six” effect triggered by amplitude.
 */
void effect_random_one_in_six_leds_by_sound(uint16_t amplitude) {
    float ratio = (float)amplitude / amp_maxn;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio <= 0.05f) {
        Turn_off_all_at_once();
        return;
    }

    Turn_off_all_at_once();
    const int group_size = 6;
    for (int start = 0; start < MAX_LED; start += group_size) {
        int rand_index = rand() % group_size;
        int led_index = start + rand_index;
        if (led_index >= MAX_LED) break;
        uint8_t brightness = (uint8_t)(ratio * 255.0f);
        uint8_t r = brightness;
        uint8_t g = rand() % brightness;
        uint8_t b = rand() % brightness;
        Set_LED(led_index, r, g, b);
    }
    Set_Brightness(5 + (int)(ratio * (NORMAL_BRIGHTNESS - 5)));
    WS2812_Send();
}

/*====================== NEW: FFT‐related ============================*/

/**
 * @brief  Record one new ADC sample into fftBufIn[], subtracting middle_point
 *         to remove DC. When fftIndex reaches FFT_BUFFER_SIZE, perform an RFFT,
 *         compute the magnitude of each complex bin, and pick the peak.
 *         The result is stored in peakHz, and fftReady=1.
 */
void record_sample_and_maybe_runFFT(uint16_t raw_adc) {
    // Convert raw 12‐bit ADC (0..4095) to float in [-1,+1], after centering around middle_point
    float centered = ((float)((int)raw_adc - (int)middle_point)) * INT16_TO_FLOAT;
    fftBufIn[fftIndex++] = centered;

    if (fftIndex >= FFT_BUFFER_SIZE) {
        // Run the RFFT in place: fftBufIn → fftBufOut
        arm_rfft_fast_f32(&fftHandler, fftBufIn, fftBufOut, 0);

        // Now compute magnitudes of bins 0..(FFT_BUFFER_SIZE/2 - 1).
        peakVal = 0.0f;
        peakHz = 0;
        uint16_t bins = FFT_BUFFER_SIZE / 2;
        for (uint16_t i = 0; i < bins; i++) {
            float re = fftBufOut[2*i];
            float im = fftBufOut[2*i + 1];
            float mag = sqrtf(re * re + im * im);
            if (mag > peakVal) {
                peakVal = mag;
                // Map bin to actual frequency: i * (Fs / N)
                peakHz = (uint16_t)((float)i * (SAMPLE_RATE_HZ / (float)FFT_BUFFER_SIZE));
            }
        }

        fftReady = 1;
        fftIndex = 0; // reset buffer index to record next block
    }
}

/*====================== NEW: Five “frequency‐aware” LED effects ============================*/

/**
 * @brief  1) Frequency→Hue effect:
 *         Map the current peakHz [0..~24kHz] onto a hue [0..360];
 *         then fade color intensity by amplitude.
 */
void effect_frequency_hue(uint16_t amplitude, uint16_t peakFrequency) {
    float ratioAmp = (float)amplitude / amp_maxn;
    if (ratioAmp > 1.0f) ratioAmp = 1.0f;

    // if very quiet or no FFT ready, switch off
    if (ratioAmp <= 0.05f || !fftReady) {
        Turn_off_all_at_once();
        return;
    }

    // Map peakFrequency (0..~24k) to hue (0..360).
    // Cap peakFrequency at e.g. 12000 Hz for full sweep.
    float freqCap = 12000.0f;
    float f = (float)peakFrequency;
    if (f > freqCap) f = freqCap;
    float hue = 360.0f * (f / freqCap);  // 0..360

    uint8_t r, g, b;
    HSV_to_RGB(hue, 1.0f, ratioAmp, &r, &g, &b);

    for (int i = 0; i < MAX_LED; i++) {
        Set_LED(i, r, g, b);
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  2) Frequency‐Bar effect:
 *         Light up a contiguous block of LEDs whose size is proportional to amplitude,
 *         colored by a gradient from blue→red based on peakFrequency.
 */
void effect_frequency_bar(uint16_t amplitude, uint16_t peakFrequency) {
    float ratioAmp = (float)amplitude / amp_maxn;
    if (ratioAmp > 1.0f) ratioAmp = 1.0f;

    int ledCount = (int)(ratioAmp * MAX_LED);
    if (ledCount < 1) ledCount = 1;

    // If FFT not ready or amplitude too small, switch off
    if (ratioAmp <= 0.05f || !fftReady) {
        Turn_off_all_at_once();
        return;
    }

    // Map peakFrequency 0..12000 → t=0..1
    float freqCap = 12000.0f;
    float t = (float)peakFrequency / freqCap;
    if (t > 1.0f) t = 1.0f;

    // Build a blue→red gradient over ledCount LEDs.
    for (int i = 0; i < MAX_LED; i++) {
        if (i < ledCount) {
            // color = interpolate between blue (0,0,255) → red (255,0,0) at position proportional to t
            uint8_t R = (uint8_t)(255.0f * t);
            uint8_t G = 0;
            uint8_t B = (uint8_t)(255.0f * (1.0f - t));
            Set_LED(i, R, G, B);
        } else {
            Set_LED(i, 0, 0, 0);
        }
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  3) Low/High color split:
 *         LEDs[0..mid) are colored “low‐freq” (green),
 *         LEDs[mid..MAX_LED) are “high‐freq” (magenta),
 *         intensity modulated by amplitude and how far basin the peak is from mid‐freq.
 */
void effect_low_high_split(uint16_t amplitude, uint16_t peakFrequency) {
    float ratioAmp = (float)amplitude / amp_maxn;
    if (ratioAmp > 1.0f) ratioAmp = 1.0f;

    // If too quiet or FFT not ready, switch off
    if (ratioAmp <= 0.05f || !fftReady) {
        Turn_off_all_at_once();
        return;
    }

    // Define split at e.g. 6000 Hz:
    float splitFreq = 6000.0f;
    int midLed = MAX_LED / 2;

    // Determine whether peak is in low or high:
    bool isHigh = ((float)peakFrequency > splitFreq);

    for (int i = 0; i < MAX_LED; i++) {
        float localBrightness = ratioAmp * 255.0f;
        if (isHigh) {
            // color = magenta: (r=brightness,g=0,b=brightness)
            if (i >= midLed) {
                Set_LED(i, (uint8_t)localBrightness, 0, (uint8_t)localBrightness);
            } else {
                Set_LED(i, 0, 0, 0);
            }
        } else {
            // color = green: (r=0,g=brightness,b=0)
            if (i < midLed) {
                Set_LED(i, 0, (uint8_t)localBrightness, 0);
            } else {
                Set_LED(i, 0, 0, 0);
            }
        }
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  4) Frequency Pulse:
 *         Blink the entire strip on/off at a rate proportional to peakFrequency.
 */
void effect_frequency_pulse(uint16_t amplitude, uint16_t peakFrequency) {
    float ratioAmp = (float)amplitude / amp_maxn;
    if (ratioAmp > 1.0f) ratioAmp = 1.0f;

    // If too quiet or FFT not ready, turn off
    if (ratioAmp <= 0.05f || !fftReady) {
        Turn_off_all_at_once();
        return;
    }

    // Map peakFrequency [100..10000] → blink period [300ms..50ms].
    float f = (float)peakFrequency;
    if (f < 100.0f) f = 100.0f;
    if (f > 10000.0f) f = 10000.0f;
    float periodMs = 300.0f - ((f - 100.0f) / (10000.0f - 100.0f)) * (300.0f - 50.0f);
    static uint32_t lastToggle = 0;
    static bool onState = false;

    uint32_t now = HAL_GetTick();
    if ((now - lastToggle) >= (uint32_t)periodMs) {
        onState = !onState;
        lastToggle = now;
    }

    if (onState) {
        // Solid white bar with brightness proportional to amplitude
        uint8_t brightness = (uint8_t)(ratioAmp * 255.0f);
        for (int i = 0; i < MAX_LED; i++) {
            Set_LED(i, brightness, brightness, brightness);
        }
    } else {
        Turn_off_all_at_once();
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

/**
 * @brief  5) Frequency Wave:
 *         Create a “traveling wave” of color whose speed depends on peakFrequency,
 *         and amplitude modulates wave amplitude.
 */
void effect_frequency_wave(uint16_t amplitude, uint16_t peakFrequency) {
    float ratioAmp = (float)amplitude / amp_maxn;
    if (ratioAmp > 1.0f) ratioAmp = 1.0f;

    // If too quiet or FFT not ready, switch off
    if (ratioAmp <= 0.05f || !fftReady) {
        Turn_off_all_at_once();
        return;
    }

    // Map peakFrequency [0..12000] → wave speed (LED shifts per call)
    float freqCap = 12000.0f;
    float f = (float)peakFrequency;
    if (f > freqCap) f = freqCap;
    // speed: 0.1 .. 2.0 positions per frame
    float speed = 0.1f + ((f / freqCap) * 1.9f);

    static float offset = 0.0f;
    offset += speed;
    if (offset >= (float)MAX_LED) offset -= (float)MAX_LED;

    for (int i = 0; i < MAX_LED; i++) {
        // Compute a sinusoid that travels: sine argument = 2π * (i/LED + offset)
        float phase = 2.0f * PI * (((float)i / (float)MAX_LED) + (offset / (float)MAX_LED));
        float waveVal = (sinf(phase) + 1.0f) / 2.0f; // [0..1]
        float brightness = waveVal * ratioAmp;      // [0..1]

        // Map waveVal to a color gradient, e.g. blue→cyan
        uint8_t r = 0;
        uint8_t g = (uint8_t)(255.0f * brightness);
        uint8_t b = (uint8_t)(255.0f * (1.0f - brightness));
        Set_LED(i, r, g, b);
    }
    Set_Brightness(NORMAL_BRIGHTNESS);
    WS2812_Send();
}

void effect_genre_classify(uint16_t amplitude, uint16_t peakFrequency) {
    float ratioAmp = (float)amplitude / amp_maxn;
    if (ratioAmp > 1.0f) ratioAmp = 1.0f;

    // Nếu biên độ quá nhỏ thì tắt LED
    if (amplitude < 700) {
        Turn_off_all_at_once();
        return;
    }

    // Nếu FFT chưa sẵn sàng (chưa đủ mẫu), tạm thời vẫn giữ màu cũ hoặc chờ đến khi fftReady=1.
    if (!fftReady) {
        // Có thể hiển thị một màu mặc định, ở đây sẽ bật trắng vừa phải
        uint8_t bright = (uint8_t)(ratioAmp * 255.0f);
        for (int i = 0; i < MAX_LED; i++) {
            Set_LED(i, bright, bright, bright);
        }
        Set_Brightness(NORMAL_BRIGHTNESS);
        WS2812_Send();
        return;
    }

    // Giả sử: nếu peakHz > 1000 Hz thì xem như “EDM”
    const uint16_t freqThreshold = 1000;
    if (peakFrequency > freqThreshold) {
        // EDM → màu nóng (gradient đỏ → da cam) brightness cao
        // Ở đây ta chỉ chọn 1 màu đỏ thẫm: (255, 60, 0)
        uint8_t R = 255;
        uint8_t G = 60;
        uint8_t B = 0;
        int brightness = NORMAL_BRIGHTNESS;      // độ sáng full
        // Nếu muốn sáng hơn nữa, có thể để brightness = NORMAL_BRIGHTNESS hoặc > tuỳ chỉnh
        for (int i = 0; i < MAX_LED; i++) {
            Set_LED(i, R, G, B);
        }
        Set_Brightness(brightness);
        WS2812_Send();
    } else {
        // Pop/Chill → màu lạnh (gradient xanh dương → cyan) brightness thấp
        // Ta chọn (0, 100, 200) làm màu chủ đạo
        uint8_t R = 0;
        uint8_t G = 100;
        uint8_t B = 200;
        int brightness = NORMAL_BRIGHTNESS / 2;  // độ sáng chỉ 50%
        for (int i = 0; i < MAX_LED; i++) {
            Set_LED(i, R, G, B);
        }
        Set_Brightness(brightness);
        WS2812_Send();
    }
}

/*======================== END OF NEW EFFECTS ================================*/

/**
  * @brief  The application entry point.
  *         We calibrate mid‐point, initialize FFT, and then in each loop:
  *         1. Read one ADC sample → get amplitude
  *         2. Feed sample to FFT buffer; optionally run RFFT when full
  *         3. Choose one effect to run. Here, we'll cycle through:
  *            • effect_sound_color (amplitude only)
  *            • effect_frequency_hue
  *            • effect_frequency_bar
  *            • effect_low_high_split
  *            • effect_frequency_pulse
  *            • effect_frequency_wave
  *         Each call lasts ~50ms.
  *
  * @retval int
  */
int main(void) {
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_TIM1_Init();
    MX_ADC1_Init();

    /* USER CODE BEGIN 2 */
    // Initialize the FFT routine for 2048‐point real FFT
    arm_rfft_fast_init_f32(&fftHandler, FFT_BUFFER_SIZE);

    // Calibrate mic “middle” to remove DC offset
    calculate_middle_point();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    uint8_t effectSelect = 0;       // which effect is currently active
    uint32_t lastEffectSwitch = 0;   // timestamp of last switch
    const uint32_t effectDuration = 5000; // switch every 5 seconds
    while (1) {
        // 1) Acquire one ADC reading
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint16_t rawSample = HAL_ADC_GetValue(&hadc1);
            amp = get_amp();                  // raw amplitude
            // send this sample into our FFT buffer
            record_sample_and_maybe_runFFT(rawSample);
        }
        HAL_ADC_Stop(&hadc1);

        // 2) If amplitude exceeds threshold and LED is off, turn on
        if (!led_is_on && amp > 700) {
            led_on_start = HAL_GetTick();
            led_is_on = true;
        }

        // 3) While LED is on, run current effect; after 50ms, switch off
        if (led_is_on) {
            // Determine which effect chunk to run:
            switch (effectSelect) {
                case 0:
                    effect_sound_color();
                    break;
                case 1:
                    effect_genre_classify(amp, peakHz);
                    break;
                case 2:
                    effect_frequency_bar(amp, peakHz);
                    break;
                case 3:
                    effect_low_high_split(amp, peakHz);
                    break;
                case 4:
                    effect_frequency_pulse(amp, peakHz);
                    break;
                case 5:
                    effect_frequency_wave(amp, peakHz);
                    break;
                default:
                    Turn_off_all_at_once();
                    break;
            }
            if ((HAL_GetTick() - led_on_start) >= 50) {
                Turn_off_all_at_once();
                led_is_on = false;
            }
        } else {
            // If no sound trigger, keep LEDs off
            Turn_off_all_at_once();
        }

        // 4) Every effectDuration ms, advance to next effect
        if ((HAL_GetTick() - lastEffectSwitch) >= effectDuration) {
            lastEffectSwitch = HAL_GetTick();
            effectSelect = (effectSelect + 1) % 6;
            // reset fftReady so that next effect waits for fresh FFT
            fftReady = 0;
        }

        HAL_Delay(1);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
