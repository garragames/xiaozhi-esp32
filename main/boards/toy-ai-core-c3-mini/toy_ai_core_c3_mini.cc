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

// No vendor-specific sequence; use driver defaults

class ToyAiCoreC3MiniBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_ = nullptr;
    bool codec_present_ = false;
    uint8_t codec_addr_ = AUDIO_CODEC_ES8311_ADDR;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    Led* led_ = nullptr;
    Button boot_button_;

    void InitCodecI2C() {
        gpio_reset_pin(LCD_CS_PIN);
        gpio_set_direction(LCD_CS_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(LCD_CS_PIN, 1);
        ESP_LOGI(TAG, "LCD CS forced HIGH");

        vTaskDelay(pdMS_TO_TICKS(100)); // Deja que el ES8311 alimente antes de sondear

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
        
        ESP_LOGI(TAG, "Init I2C (SDA=%d, SCL=%d) @ 100kHz", AUDIO_CODEC_I2C_SDA_PIN, AUDIO_CODEC_I2C_SCL_PIN);
        gpio_reset_pin(LCD_CS_PIN);
        gpio_set_direction(LCD_CS_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(LCD_CS_PIN, 1);
        ESP_LOGI(TAG, "LCD CS forced HIGH");

        vTaskDelay(pdMS_TO_TICKS(50)); // Permitir power-up del codec

        ESP_LOGI(TAG, "Init I2C (SDA=%d, SCL=%d) driver_ng @ ~100kHz", AUDIO_CODEC_I2C_SDA_PIN, AUDIO_CODEC_I2C_SCL_PIN);
        if (i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_) == ESP_OK) {
            uint8_t addrs[] = {0x1B, 0x1A, 0x19, 0x18};
            for (uint8_t a : addrs) {
                if (i2c_master_probe(codec_i2c_bus_, a, 200) == ESP_OK) {
                    codec_addr_ = a;
                    codec_present_ = true;
                    ESP_LOGI(TAG, "ES8311 detected on I2C addr 0x%02x", a);
                    break;
                }
            }
            if (!codec_present_) {
                ESP_LOGE(TAG, "ES8311 NOT detected on addrs 0x18-0x1B");
            }
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
        io_config.spi_mode = 0; // GC9A01 trabaja en modo 0
        io_config.pclk_hz = 10 * 1000 * 1000; // 10MHz para estabilidad
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io_));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = LCD_RST_PIN;
        panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
        panel_config.bits_per_pixel = 16;

        panel_config.vendor_config = nullptr; // usar init por defecto
        
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io_, &panel_config, &panel_));

        esp_lcd_panel_reset(panel_);
        esp_lcd_panel_init(panel_);
        
        // Invertir como en el test-pin para evitar negativo de colores
        esp_lcd_panel_invert_color(panel_, true); 
        
        esp_lcd_panel_disp_on_off(panel_, true);

        // Patrón rápido de verificación de color (se ejecuta una vez al arranque)
        auto fill_color = [&](uint16_t color) {
            static std::vector<uint16_t> line(DISPLAY_WIDTH, color);
            for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
                esp_lcd_panel_draw_bitmap(panel_, 0, y, DISPLAY_WIDTH, y + 1, line.data());
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        };
        // Prueba breve: solo rojo para verificar color y no demorar arranque
        ESP_LOGI(TAG, "Test color RED");
        fill_color(0xF800); // rojo

        display_ = new SpiLcdDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, false);
    }

public:
    ToyAiCoreC3MiniBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        esp_log_level_set("gc9a01", ESP_LOG_DEBUG);
        
        vTaskDelay(pdMS_TO_TICKS(1000));

        InitCodecI2C(); 
        InitializeSpi();               
        InitializeGc9a01Display();
        
        if (BUILTIN_LED_GPIO != GPIO_NUM_NC) {
            led_ = new SingleLed(BUILTIN_LED_GPIO);
        }

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
    virtual Led* GetLed() override {
        if (led_ == nullptr) {
            static NoLed no_led;
            return &no_led;
        }
        return led_;
    }
    
    virtual AudioCodec* GetAudioCodec() override {
        if (codec_i2c_bus_ == nullptr || !codec_present_) {
            static NoAudioCodecDuplex i2s_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DSDIN, AUDIO_I2S_GPIO_ASDOUT);
            return &i2s_codec;
        }
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_LRCK, AUDIO_I2S_GPIO_DSDIN, AUDIO_I2S_GPIO_ASDOUT,
            AUDIO_CODEC_PA_PIN, codec_addr_, true /*use_mclk*/);
        return &audio_codec;
    }

    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        level = 100; charging = false; discharging = true; return false;
    }
    virtual std::string GetBoardType() override { return "toy-ai-core-c3-mini"; }
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override { WifiBoard::SetPowerSaveLevel(level); }
};

DECLARE_BOARD(ToyAiCoreC3MiniBoard);
