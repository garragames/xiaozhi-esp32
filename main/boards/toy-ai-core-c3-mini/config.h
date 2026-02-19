#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Frecuencia de audio
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

/* ---------- I2S (ES8311 audio) ---------- */
#define AUDIO_I2S_GPIO_MCLK     GPIO_NUM_10
#define AUDIO_I2S_GPIO_BCLK     GPIO_NUM_8
#define AUDIO_I2S_GPIO_LRCK     GPIO_NUM_6
#define AUDIO_I2S_GPIO_DSDIN    GPIO_NUM_5
#define AUDIO_I2S_GPIO_ASDOUT   GPIO_NUM_7

#define AUDIO_I2S_GPIO_SCLK     AUDIO_I2S_GPIO_BCLK 
#define AUDIO_I2S_GPIO_WS       AUDIO_I2S_GPIO_LRCK

/* ---------- I2C (ES8311 control) ---------- */
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_3
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_0
#define AUDIO_CODEC_ES8311_ADDR  0x18
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_NC

/* ---------- LCD SPI (GC9A01) ---------- */
#define LCD_HOST                SPI2_HOST
#define LCD_MOSI_PIN            GPIO_NUM_21
#define LCD_SCLK_PIN            GPIO_NUM_0
#define LCD_CS_PIN              GPIO_NUM_20
#define LCD_DC_PIN              GPIO_NUM_1
#define LCD_RST_PIN             GPIO_NUM_4 // Reseteo dedicado (probado)
#define LCD_BL_PIN              GPIO_NUM_NC // backlight fijo en hardware

/* ---------- Otros ---------- */
#define BUILTIN_LED_GPIO        GPIO_NUM_NC
#define BOOT_BUTTON_GPIO        GPIO_NUM_9

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false

#endif // _BOARD_CONFIG_H_
