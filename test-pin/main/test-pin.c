#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_gc9a01.h"

static const char *TAG = "PIN_PROBE";

/* ---- Ajusta si quieres incluir pines “peligrosos” ----
   Para mantener el USB estable, por default EXCLUYO GPIO18/19.
   También excluyo GPIO9 (BOOT) y GPIO2 (strap + WS2812 típico). */
static const int candidate_pins[] = {
    0, 1, 3, 4, 5, 6, 7, 8, 10, 20, 21
    // Si quieres probar más: 18, 19 (pero puede tumbar el USB CDC)
};

static bool i2c_probe_addr(i2c_port_t port, uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(30));
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK);
}

static void scan_i2c_for_es8311(void) {
    ESP_LOGW(TAG, "=== I2C scan (bus pins) buscando ES8311 (0x18/0x19/0x1A/0x1B comunes) ===");

    for (int i = 0; i < (int)(sizeof(candidate_pins)/sizeof(candidate_pins[0])); i++) {
        for (int j = 0; j < (int)(sizeof(candidate_pins)/sizeof(candidate_pins[0])); j++) {
            if (i == j) continue;
            int sda = candidate_pins[i];
            int scl = candidate_pins[j];

            // Configura I2C
            i2c_config_t conf = {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = sda,
                .scl_io_num = scl,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master.clk_speed = 400000
            };

            i2c_driver_delete(I2C_NUM_0);
            if (i2c_param_config(I2C_NUM_0, &conf) != ESP_OK) continue;
            if (i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0) != ESP_OK) continue;

            // Probar direcciones típicas ES8311
            bool hit18 = i2c_probe_addr(I2C_NUM_0, 0x18);
            bool hit19 = i2c_probe_addr(I2C_NUM_0, 0x19);
            bool hit1A = i2c_probe_addr(I2C_NUM_0, 0x1A);
            bool hit1B = i2c_probe_addr(I2C_NUM_0, 0x1B);

            if (hit18 || hit19 || hit1A || hit1B) {
                ESP_LOGE(TAG, ">>> ENCONTRADO I2C valido: SDA=%d SCL=%d  (ACK: 0x18=%d 0x19=%d 0x1A=%d 0x1B=%d)",
                         sda, scl, hit18, hit19, hit1A, hit1B);
                return;
            }
        }
    }

    ESP_LOGE(TAG, "No encontré ES8311 por I2C en el set de pines candidato. Si quieres, incluyo GPIO18/19 en candidates.");
}

/* Dibuja un color sólido (RGB565) */
static esp_err_t fill_color(esp_lcd_panel_handle_t panel, uint16_t color) {
    static uint16_t line[240];
    for (int i = 0; i < 240; i++) line[i] = color;

    for (int y = 0; y < 240; y++) {
        esp_err_t err = esp_lcd_panel_draw_bitmap(panel, 0, y, 240, y+1, line);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

/* Brute force LCD: asume MOSI/CS pueden ser 21/20 o intercambiados. */
static void brute_force_lcd_gc9a01(void) {
    ESP_LOGW(TAG, "=== Brute force LCD GC9A01: observar pantalla; cuando veas colores, apunta el ultimo TRY ===");

    const int pin_set[] = {0,1,3,4,5,6,7,8,10,20,21};
    const int N = sizeof(pin_set)/sizeof(pin_set[0]);

    // Probamos dos escenarios frecuentes para MOSI/CS:
    const int mosi_options[] = {21, 20};
    const int cs_options[]   = {20, 21};

    for (int mo = 0; mo < 2; mo++) {
        int mosi = mosi_options[mo];
        int cs   = cs_options[mo];

        for (int si = 0; si < N; si++) {
            int sclk = pin_set[si];
            if (sclk == mosi || sclk == cs) continue;

            for (int di = 0; di < N; di++) {
                int dc = pin_set[di];
                if (dc == mosi || dc == cs || dc == sclk) continue;

                for (int ri = 0; ri < N; ri++) {
                    int rst = pin_set[ri];
                    if (rst == mosi || rst == cs || rst == sclk || rst == dc) continue;

                    ESP_LOGW(TAG, "TRY LCD: MOSI=%d SCLK=%d CS=%d DC=%d RST=%d (si ves colores, ESTE es el pinmap)",
                             mosi, sclk, cs, dc, rst);

                    // SPI bus init
                    spi_bus_config_t buscfg = {
                        .mosi_io_num = mosi,
                        .miso_io_num = -1,
                        .sclk_io_num = sclk,
                        .quadwp_io_num = -1,
                        .quadhd_io_num = -1,
                        .max_transfer_sz = 240 * 40 * 2
                    };

                    spi_bus_free(SPI2_HOST);
                    if (spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) {
                        continue;
                    }

                    esp_lcd_panel_io_handle_t io_handle = NULL;
                    esp_lcd_panel_handle_t panel_handle = NULL;

                    esp_lcd_panel_io_spi_config_t io_config = {
                        .dc_gpio_num = dc,
                        .cs_gpio_num = cs,
                        .pclk_hz = 10 * 1000 * 1000,   // conservador
                        .lcd_cmd_bits = 8,
                        .lcd_param_bits = 8,
                        .spi_mode = 0,
                        .trans_queue_depth = 10,
                    };

                    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle) != ESP_OK) {
                        spi_bus_free(SPI2_HOST);
                        continue;
                    }

                    esp_lcd_panel_dev_config_t panel_config = {
                        .reset_gpio_num = rst,
                        .color_space = ESP_LCD_COLOR_SPACE_RGB,
                        .bits_per_pixel = 16,
                    };

                    if (esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle) != ESP_OK) {
                        esp_lcd_panel_io_del(io_handle);
                        spi_bus_free(SPI2_HOST);
                        continue;
                    }

                    esp_lcd_panel_reset(panel_handle);
                    esp_lcd_panel_init(panel_handle);
                    esp_lcd_panel_disp_on_off(panel_handle, true);

                    // Si esta combinación es correcta, verás colores
                    fill_color(panel_handle, 0xF800); // rojo
                    vTaskDelay(pdMS_TO_TICKS(700));
                    fill_color(panel_handle, 0x07E0); // verde
                    vTaskDelay(pdMS_TO_TICKS(700));
                    fill_color(panel_handle, 0x001F); // azul
                    vTaskDelay(pdMS_TO_TICKS(700));

                    // Cleanup para siguiente intento
                    esp_lcd_panel_del(panel_handle);
                    esp_lcd_panel_io_del(io_handle);
                    spi_bus_free(SPI2_HOST);
                }
            }
        }
    }

    ESP_LOGE(TAG, "Terminé brute force sin ver colores. Si el BL es controlado por GPIO, hay que encontrar BL/POWER primero.");
}

void app_main(void) {
    ESP_LOGI(TAG, "Arranque pin-probe. NOTA: usa monitor --no-reset y power-cycle con PWR.");

    scan_i2c_for_es8311();

    vTaskDelay(pdMS_TO_TICKS(1000));
    brute_force_lcd_gc9a01();

    ESP_LOGW(TAG, "Fin.");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
