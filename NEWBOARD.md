# How to Add a New Board Based on an Existing One

This guide explains how to create a new board definition by duplicating and modifying an existing one, ensuring a successful build with the `scripts/release.py` script from the start.

We will use the creation of the `kotty` board, based on `bread-compact-wifi-lcd`, as an example.

## Step-by-Step Guide

### 1. Duplicate the Existing Board Directory

First, find the directory of the board you want to use as a base in `main/boards/`. Copy and paste it, renaming it to match your new board's name.

**Example:**
Copy `main/boards/bread-compact-wifi-lcd` and rename the copy to `main/boards/kotty`.

### 2. Rename Core Files and Update Class Names

Inside your new board's directory, rename the main `.cc` file to match the board name. Then, open this file and change the class name and the `TAG` definition to reflect the new board name.

**Example:**
1.  Rename `main/boards/kotty/compact_wifi_board_lcd.cc` to `main/boards/kotty/kotty.cc`.
2.  Open `kotty.cc` and make the following changes:
    *   Change `#define TAG "CompactWifiBoardLCD"` to `#define TAG "Kotty"`.
    *   Change `class CompactWifiBoardLCD : public WifiBoard` to `class Kotty : public WifiBoard`.
    *   Change the constructor from `CompactWifiBoardLCD()` to `Kotty()`.
    *   Update the final declaration from `DECLARE_BOARD(CompactWifiBoardLCD);` to `DECLARE_BOARD(Kotty);`.

### 3. Update Build System Files

You need to inform the build system about your new board.

#### a. `main/CMakeLists.txt`

Add an `elseif` block for your new board. This tells the build system which source file to compile for your board type.

**Example:**
```cmake
# In main/CMakeLists.txt, add this block:
elseif(CONFIG_BOARD_TYPE_KOTTY)
    set(BOARD_SRC_DIR boards/kotty)
    set(BOARD_COMPONENT_SRCS "${BOARD_SRC_DIR}/kotty.cc")
```

#### b. `main/Kconfig.projbuild`

This file controls the board selection menu. You need to make two changes here:

1.  **Add the new board as a choice:** Find the `choice BOARD_TYPE` section and add a `config` option for your new board. You can also set it as the `default`.
2.  **CRITICAL:** **Update feature dependencies.** If your board uses specific hardware like an LCD screen, you must add your new board's `CONFIG_BOARD_TYPE_*` flag to the `depends on` clause of that feature. **Forgetting this step is a common cause of build failures.**

**Example:**
```kconfig
# In main/Kconfig.projbuild:

# 1. Add the board to the choice list
choice BOARD_TYPE
    # ... existing boards ...
    config BOARD_TYPE_BREAD_COMPACT_WIFI_LCD
        bool "Bread Compact Wifi LCD"
    config BOARD_TYPE_KOTTY # <-- ADD THIS
        bool "Kotty"        # <-- ADD THIS

endchoice

# 2. Update dependencies for features (e.g., LCD Type)
choice DISPLAY_LCD_TYPE
    depends on BOARD_TYPE_BREAD_COMPACT_WIFI_LCD || BOARD_TYPE_KOTTY # <-- ADD YOUR BOARD HERE
    # ... other options ...
```

### 4. Configure the Board's `config.json`

Inside your new board's directory, edit the `config.json` file. This file is used by the `release.py` script to generate the final build configuration (`sdkconfig`).

Make sure to:
*   Set the `name` to your new board's name.
*   Update `CONFIG_BOARD_TYPE_*` to be specific to your new board (e.g., `"CONFIG_BOARD_TYPE_KOTTY": "y"`).
*   Remove the old board's type flag (e.g., `"CONFIG_BOARD_TYPE_BREAD_COMPACT_WIFI_LCD": null`).
*   Ensure all hardware configurations (like `CONFIG_LCD_GC9A01_240X240`) are correctly set.

**Example (`main/boards/kotty/config.json`):**
```json
{
  "name": "kotty",
  "version": "v2.2.1",
  "files": [
    {
      "offset": "0x800000",
      "path": "main/boards/kotty/assets.bin",
      "size": "auto"
    }
  ],
  "config": {
    "CONFIG_BOARD_TYPE_KOTTY": "y",
    "CONFIG_BOARD_TYPE_BREAD_COMPACT_WIFI_LCD": null,
    "CONFIG_LCD_GC9A01_240X240": "y",
    "CONFIG_LCD_TYPE_GC9A01_SERIAL": "y",
    "CONFIG_EXAMPLE_THEME_1_0_INCH_ROUND": "y",
    "CONFIG_EXAMPLE_THEME_FONT_SIZE_24": "y"
  }
}
```

### 5. Build the New Board

After following all the steps, you can now build the firmware for your new board without issues.

```bash
python scripts/release.py your_new_board_name
```

**Example:**
```bash
python scripts/release.py kotty
```

This will generate the final firmware and a release `.zip` file in the `releases/` directory.
