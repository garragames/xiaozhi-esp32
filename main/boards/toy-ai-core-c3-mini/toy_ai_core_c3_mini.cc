#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "led/single_led.h"
#include "config.h"
#include "power_save_timer.h"

#include <esp_log.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_gc9a01.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/spi_master.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_efuse.h>
#include <esp_efuse_table.h>
#include <vector>

#define TAG "ToyAiCoreC3Mini"

static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
    {0xfe, (uint8_t[]){0x00}, 0, 0},
    {0xef, (uint8_t[]){0x00}, 0, 0},
    {0xb0, (uint8_t[]){0xc0}, 1, 0},
    {0xb1, (uint8_t[]){0x80}, 1, 0},
    {0xb2, (uint8_t[]){0x27}, 1, 0},
    {0xb3, (uint8_t[]){0x13}, 1, 0},
    {0xb6, (uint8_t[]){0x19}, 1, 0},
    {0xb7, (uint8_t[]){0x05}, 1, 0},
    {0xac, (uint8_t[]){0xc8}, 1, 0},
    {0xab, (uint8_t[]){0x0f}, 1, 0},
    {0x3a, (uint8_t[]){0x05}, 1, 0},
    {0xb4, (uint8_t[]){0x04}, 1, 0},
    {0xa8, (uint8_t[]){0x08}, 1, 0},
    {0xb8, (uint8_t[]){0x08}, 1, 0},
    {0xea, (uint8_t[]){0x02}, 1, 0},
    {0xe8, (uint8_t[]){0x2A}, 1, 0},
    {0xe9, (uint8_t[]){0x47}, 1, 0},
    {0xe7, (uint8_t[]){0x5f}, 1, 0},
    {0xc6, (uint8_t[]){0x21}, 1, 0},
    {0xc7, (uint8_t[]){0x15}, 1, 0},
    {0xf0, (uint8_t[]){0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C, 0x04, 0x12, 0x14, 0x1f}, 14, 0},
    {0xf1, (uint8_t[]){0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D, 0x0C, 0x1A, 0x14, 0x1E}, 14, 0},
    {0xf4, (uint8_t[]){0x00, 0x00, 0xFF}, 3, 0},
    {0xba, (uint8_t[]){0xFF, 0xFF}, 2, 0},
};

class ToyAiCoreC3MiniBoard : public WifiBoard {
private:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    Led* led_ = nullptr;
    Button boot_button_;

    void ConfigureCodecAndReleaseI2C() {
        gpio_reset_pin(LCD_CS_PIN);
        gpio_set_direction(LCD_CS_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(LCD_CS_PIN, 1);
        ESP_LOGI(TAG, "LCD CS forced HIGH");

        i2c_master_bus_handle_t i2c_bus = nullptr;
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = { .enable_internal_pullup = 1 },
        };
        
        ESP_LOGI(TAG, "Init I2C (SDA=%d, SCL=%d) @ 50kHz", AUDIO_CODEC_I2C_SDA_PIN, AUDIO_CODEC_I2C_SCL_PIN);
        // Bajar velocidad I2C a 50kHz para mayor robustez en pin compartido
        if (i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus) == ESP_OK) {
            {
                Es8311AudioCodec codec(i2c_bus, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                    GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC,
                    GPIO_NUM_NC, AUDIO_CODEC_ES8311_ADDR);
                codec.SetOutputVolume(70);
                ESP_LOGI(TAG, "Codec Configured");
            }
            i2c_del_master_bus(i2c_bus);
            ESP_LOGI(TAG, "I2C Released");
        } else {
            ESP_LOGE(TAG, "I2C Init Failed");
        }
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = LCD_MOSI_PIN;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = LCD_SCLK_PIN;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeGc9a01Display() {
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = LCD_CS_PIN;
        io_config.dc_gpio_num = LCD_DC_PIN;
        io_config.spi_mode = 3; // PROBAR MODO 3 (Fix para rayas/sincronía)
        io_config.pclk_hz = 10 * 1000 * 1000; // 10MHz (Reducido para pin compartido)
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io_));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = LCD_RST_PIN;
        panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR;
        panel_config.bits_per_pixel = 16;
        
        gc9a01_vendor_config_t gc9107_vendor_config = {
            .init_cmds = gc9107_lcd_init_cmds,
            .init_cmds_size = sizeof(gc9107_lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
        };
        panel_config.vendor_config = &gc9107_vendor_config;
        
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io_, &panel_config, &panel_));

        esp_lcd_panel_reset(panel_);
        esp_lcd_panel_init(panel_);
        
        // Inversión: false (Ya que se veía gris/negativo)
        esp_lcd_panel_invert_color(panel_, false); 
        
        esp_lcd_panel_disp_on_off(panel_, true);

        // Prueba de color
        ESP_LOGI(TAG, "Test: Filling screen with RED");
        std::vector<uint16_t> color_buf(DISPLAY_WIDTH * 10, 0xF800);
        for (int y = 0; y < DISPLAY_HEIGHT; y += 10) {
            esp_lcd_panel_draw_bitmap(panel_, 0, y, DISPLAY_WIDTH, y + 10, color_buf.data());
        }
        vTaskDelay(pdMS_TO_TICKS(500));

        display_ = new SpiLcdDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, false);
    }

public:
    ToyAiCoreC3MiniBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        esp_log_level_set("gc9a01", ESP_LOG_DEBUG);
        
        vTaskDelay(pdMS_TO_TICKS(1000));

        ConfigureCodecAndReleaseI2C(); 
        InitializeSpi();               
        InitializeGc9a01Display();
        
        led_ = new SingleLed(BUILTIN_LED_GPIO);

        boot_button_.OnClick([this]() {
            Application::GetInstance().ToggleChatState();
        });

        esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
        ESP_LOGI(TAG, "ToyAiCoreC3MiniBoard Initialized");
    }

    virtual Display* GetDisplay() override {
        return display_ ? display_ : (Display*)(new NoDisplay());
    }

    virtual Backlight* GetBacklight() override { return nullptr; }
    virtual Led* GetLed() override { return led_; }
    
    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecDuplex i2s_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DSDIN, AUDIO_I2S_GPIO_ASDOUT);
        return &i2s_codec;
    }

    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        level = 100; charging = false; discharging = true; return false;
    }
    virtual std::string GetBoardType() override { return "toy-ai-core-c3-mini"; }
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override { WifiBoard::SetPowerSaveLevel(level); }
};

DECLARE_BOARD(ToyAiCoreC3MiniBoard);
