#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// I2S Pins - Disabled for now to prevent boot conflict
#define AUDIO_I2S_GPIO_SCLK GPIO_NUM_NC 
#define AUDIO_I2S_GPIO_LRCK GPIO_NUM_NC 
#define AUDIO_I2S_GPIO_DSDIN GPIO_NUM_NC
#define AUDIO_I2S_GPIO_ASDOUT GPIO_NUM_NC

// I2C Pins for Codec - Disabled for now
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_NC
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_NC
#define AUDIO_CODEC_ES8311_ADDR  0x18

// LCD Pins (SPI) - Safe Config Attempt
#define LCD_SCLK_PIN            GPIO_NUM_6
#define LCD_MOSI_PIN            GPIO_NUM_7
#define LCD_CS_PIN              GPIO_NUM_10
#define LCD_DC_PIN              GPIO_NUM_5
#define LCD_RST_PIN             GPIO_NUM_4
#define LCD_BL_PIN              GPIO_NUM_NC // Disable BL initially

#define BUILTIN_LED_GPIO        GPIO_NUM_8
#define BOOT_BUTTON_GPIO        GPIO_NUM_9

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false

#endif // _BOARD_CONFIG_H_
