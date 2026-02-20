#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Audio pins from xmini-c3 (verified working)
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_10
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_6
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_8
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_7
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_5

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_11
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_3
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_4
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

#define BUILTIN_LED_GPIO        GPIO_NUM_2
#define BOOT_BUTTON_GPIO        GPIO_NUM_9

// Display pins (GC9A01 via SPI)
#define LCD_HOST                SPI2_HOST
#define LCD_MOSI_PIN            GPIO_NUM_21
#define LCD_SCLK_PIN            GPIO_NUM_0
#define LCD_CS_PIN              GPIO_NUM_20
#define LCD_DC_PIN              GPIO_NUM_1
#define LCD_RST_PIN             GPIO_NUM_NC
#define LCD_BL_PIN              GPIO_NUM_NC

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false

#endif // _BOARD_CONFIG_H_
