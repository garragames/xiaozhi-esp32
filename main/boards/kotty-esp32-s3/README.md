# Kotty ESP32-S3

Placa de desarrollo de alto rendimiento basada en el chip **ESP32-S3**, diseñada para asistentes de voz avanzados con pantalla circular y capacidades de IA locales.

## Características de Hardware

- **MCU:** Espressif ESP32-S3 (Xtensa Dual Core)
- **Pantalla:** GC9A01 Circular LCD (240x240 px)
  - Interfaz: SPI (Host SPI3)
  - Pines: MOSI: 47, SCLK: 21, CS: 41, DC: 40, RST: 45, BL: 42
- **Audio:** Interfaz I2S Genérica (Simplex/Duplex)
  - Configuración flexible para micrófonos I2S/PDM y amplificadores I2S (MAX98357A, etc.) sin codec de control I2C dedicado.
  - Pines (Simplex Defecto): 
    - Micrófono: WS: 4, SCK: 5, DIN: 6
    - Altavoz: DOUT: 7, BCLK: 15, LRCK: 16
- **Periféricos:**
  - Botón Boot: GPIO 0
  - LED Integrado: GPIO 48
  - Control de Lámpara (Relé/MOSFET): GPIO 18

## Configuración de Software

- **Motor de Audio:** Soporta procesamiento de audio avanzado (AEC, BSS) gracias a la memoria y potencia del S3.
- **Wake Word:** Activado (WN9).
- **Gráficos:** LVGL con aceleración hardware (si disponible) y buffer completo.
- **Idioma:** Español (ES_ES).
- **Assets:** Utiliza una partición de assets personalizada para recursos gráficos y de audio.

## Compilación y Flasheo

### Opción 1: Script de Release (Recomendado)
Este script gestiona la configuración completa y la generación de los archivos de recursos necesarios.

```bash
python3 scripts/release.py kotty-esp32-s3
```

El archivo resultante se encontrará en la carpeta `releases/`.

### Opción 2: Compilación Manual con IDF

1.  Configurar el target:
    ```bash
    idf.py set-target esp32s3
    ```
2.  Compilar definiendo el tipo de placa:
    ```bash
    idf.py -D BOARD_TYPE=kotty-esp32-s3 build
    ```
3.  Flashear:
    ```bash
    idf.py -D BOARD_TYPE=kotty-esp32-s3 flash monitor
    ```
