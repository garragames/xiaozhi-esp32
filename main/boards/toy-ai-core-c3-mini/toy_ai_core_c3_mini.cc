#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "codecs/dummy_audio_codec.h" // Fallback seguro
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
#include <driver/spi_master.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_efuse.h>
#include <esp_efuse_table.h>

#define TAG "ToyAiCoreC3Mini"

class ToyAiCoreC3MiniBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_ = nullptr;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    Backlight* backlight_ = nullptr;
    Led* led_ = nullptr;
    Button boot_button_;

    void InitializeCodecI2c() {
        // Verificar si los pines I2C son validos antes de inicializar
        if (AUDIO_CODEC_I2C_SDA_PIN < 0 || AUDIO_CODEC_I2C_SCL_PIN < 0) {
            ESP_LOGW(TAG, "I2C pins not configured, skipping codec initialization");
            return;
        }

        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        esp_err_t ret = i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
            codec_i2c_bus_ = nullptr;
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
        esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        }
    }

    void InitializeGc9a01Display() {
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = LCD_CS_PIN;
        io_config.dc_gpio_num = LCD_DC_PIN;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        
        esp_err_t ret = esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io_);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
            return;
        }

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = LCD_RST_PIN;
        panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR;
        panel_config.bits_per_pixel = 16;
        
        ret = esp_lcd_new_panel_gc9a01(panel_io_, &panel_config, &panel_);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create GC9A01 panel: %s", esp_err_to_name(ret));
            return;
        }

        esp_lcd_panel_reset(panel_);
        esp_lcd_panel_init(panel_);
        esp_lcd_panel_invert_color(panel_, true);
        esp_lcd_panel_disp_on_off(panel_, true);

        display_ = new SpiLcdDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, false);
    }

public:
    ToyAiCoreC3MiniBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        // Safety delay to allow strapping pins (GPIO2, GPIO8, GPIO9) to stabilize
        vTaskDelay(pdMS_TO_TICKS(1000));

        InitializeCodecI2c();
        InitializeSpi();
        InitializeGc9a01Display();
        
        if (LCD_BL_PIN >= 0) {
            backlight_ = new PwmBacklight(LCD_BL_PIN, false);
        }
        
        if (BUILTIN_LED_GPIO >= 0) {
            led_ = new SingleLed(BUILTIN_LED_GPIO);
        }

        boot_button_.OnClick([this]() {
            Application::GetInstance().ToggleChatState();
        });

        // Configuración de EFUSE crítica para variantes C3 Mini
        esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
        
        ESP_LOGI(TAG, "ToyAiCoreC3MiniBoard Initialized - Safe Boot Complete");
    }

    virtual Display* GetDisplay() override {
        if (display_ == nullptr) {
            static NoDisplay no_display;
            return &no_display;
        }
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        return backlight_;
    }

    virtual Led* GetLed() override {
        if (led_ == nullptr) {
            static NoLed no_led;
            return &no_led;
        }
        return led_;
    }

    virtual AudioCodec* GetAudioCodec() override {
        // Fallback seguro: Si no hay bus I2C (configurado como NC), devolvemos DummyAudioCodec
        if (codec_i2c_bus_ == nullptr) {
            static DummyAudioCodec dummy_audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE);
            return &dummy_audio_codec;
        }
        
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            GPIO_NUM_NC, AUDIO_I2S_GPIO_SCLK, AUDIO_I2S_GPIO_LRCK, AUDIO_I2S_GPIO_DSDIN, AUDIO_I2S_GPIO_ASDOUT,
            GPIO_NUM_NC, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        level = 100;
        charging = false;
        discharging = true;
        return false;
    }

    virtual std::string GetBoardType() override {
        return "toy-ai-core-c3-mini";
    }

    virtual void SetPowerSaveLevel(PowerSaveLevel level) override {
        WifiBoard::SetPowerSaveLevel(level);
    }
};

DECLARE_BOARD(ToyAiCoreC3MiniBoard);
