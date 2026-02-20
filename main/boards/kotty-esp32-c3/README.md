# Kotty ESP32-C3

Placa de desarrollo basada en el chip **ESP32-C3**, diseñada para asistentes de voz compactos con pantalla circular. Esta configuración está altamente optimizada para funcionar dentro de las limitaciones de memoria RAM del ESP32-C3 (400KB SRAM).

## Características de Hardware

- **MCU:** Espressif ESP32-C3 (RISC-V Single Core)
- **Pantalla:** GC9A01 Circular LCD (240x240 px)
  - Interfaz: SPI
  - Pines: MOSI: 21, SCLK: 0, CS: 20, DC: 1, RST: NC
- **Audio:** Codec ES8311
  - Interfaz: I2S (Audio) + I2C (Control)
  - Pines I2S: MCLK: 10, BCLK: 8, WS: 6, DOUT: 5, DIN: 7
  - Pines I2C: SDA: 3, SCL: 4
  - Pin PA (Amplificador): 11
- **Periféricos:**
  - Botón Boot/Acción: GPIO 9
  - LED Integrado: GPIO 2

## Configuración de Software

Esta placa incluye optimizaciones específicas para la gestión de memoria:
- **Profundidad de Color:** 16-bit (RGB565) con *Byte Swap* activado en LVGL.
- **Wake Word:** Modelo **Alexa (WN9)**, configurado para ejecutarse directamente desde la memoria Flash (`CONFIG_ESP_SR_WN_MODEL_IN_FLASH`) para ahorrar RAM.
- **Frecuencia de Audio:** 16kHz nativo para evitar buffers de resampleo.
- **Buffers WiFi:** Reducidos al mínimo viable para liberar heap.
- **Idioma:** Español (ES_ES).

## Compilación y Flasheo

### Opción 1: Script de Release (Recomendado)
Este script configura automáticamente el entorno, aplica los parches de configuración (`sdkconfig`) y genera un binario unificado.

```bash
python3 scripts/release.py kotty-esp32-c3
```

El archivo resultante se encontrará en la carpeta `releases/`.

### Opción 2: Compilación Manual con IDF
Si prefieres usar los comandos estándar de ESP-IDF:

1.  Configurar el target:
    ```bash
    idf.py set-target esp32c3
    ```
2.  Compilar definiendo el tipo de placa:
    ```bash
    idf.py -D BOARD_TYPE=kotty-esp32-c3 build
    ```
3.  Flashear:
    ```bash
    idf.py -D BOARD_TYPE=kotty-esp32-c3 flash monitor
    ```
