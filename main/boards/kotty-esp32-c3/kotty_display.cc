#include "kotty_display.h"
#include "lvgl_theme.h"
#include "font_awesome.h"
#include "assets/lang_config.h"
#include <esp_log.h>

#define TAG "KottyDisplay"

LV_FONT_DECLARE(font_awesome_30_4);
extern const lv_img_dsc_t boot_logo;
extern const lv_img_dsc_t kotty_standby;

KottyDisplay::KottyDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                           int width, int height, int offset_x, int offset_y,
                           bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy) {
    // Initialize our specific UI
    SetupUI();
}

void KottyDisplay::SetupUI() {
    DisplayLockGuard lock(this);
    
    // Clean current screen (remove elements created by base class)
    lv_obj_clean(lv_screen_active());

    // Reset base class pointers (LcdDisplay) - Objects deleted by lv_obj_clean
    container_ = nullptr;
    top_bar_ = nullptr;
    status_bar_ = nullptr;
    content_ = nullptr;
    side_bar_ = nullptr;
    bottom_bar_ = nullptr;
    preview_image_ = nullptr;
    emoji_label_ = nullptr;
    emoji_image_ = nullptr;
    emoji_box_ = nullptr;
    chat_message_label_ = nullptr;
    
    // Reset base class pointers (LvglDisplay) - CRITICAL to avoid dangling pointers
    network_label_ = nullptr;
    status_label_ = nullptr;
    notification_label_ = nullptr;
    mute_label_ = nullptr;
    battery_label_ = nullptr;
    low_battery_popup_ = nullptr;
    low_battery_label_ = nullptr;

    auto screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    // Image for Logo and Eyes (Centered)
    kotty_image_ = lv_image_create(screen);
    lv_image_set_src(kotty_image_, &boot_logo);
    lv_obj_center(kotty_image_);

    // Label for FontAwesome icons (Bottom Center)
    state_icon_label_ = lv_label_create(screen);
    lv_obj_set_style_text_font(state_icon_label_, &font_awesome_30_4, 0);
    lv_obj_set_style_text_color(state_icon_label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(state_icon_label_, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_label_set_text(state_icon_label_, ""); // Initially empty
}

void KottyDisplay::SetEmotion(const char* emotion) {
    DisplayLockGuard lock(this);
    
    if (kotty_image_ == nullptr) return;

    // Switch to eyes if we were showing logo (first time we get an emotion/state)
    const void* src = lv_image_get_src(kotty_image_);
    if (src == &boot_logo) {
        lv_image_set_src(kotty_image_, &kotty_standby);
    }

    // Intentionally ignore the emotion content.
    // We do NOT want to display happy/sad/angry icons.
    // The state_icon_label_ is exclusively for system status (mic, speaker, etc.)
}

void KottyDisplay::SetStatus(const char* status) {
    DisplayLockGuard lock(this);
    if (status == nullptr || kotty_image_ == nullptr) return;

    // Switch to eyes if we were showing logo (and not just initializing)
    const void* src = lv_image_get_src(kotty_image_);
    if (src == &boot_logo && (strcmp(status, Lang::Strings::INITIALIZING) != 0)) {
        lv_image_set_src(kotty_image_, &kotty_standby);
    }

    if (strcmp(status, Lang::Strings::LISTENING) == 0) {
        lv_label_set_text(state_icon_label_, FONT_AWESOME_MICROPHONE);
        lv_obj_set_style_text_color(state_icon_label_, lv_color_hex(0xFF0000), 0); // RED
    } else if (strcmp(status, Lang::Strings::SPEAKING) == 0) {
        lv_label_set_text(state_icon_label_, FONT_AWESOME_VOLUME_HIGH);
        lv_obj_set_style_text_color(state_icon_label_, lv_color_hex(0x00FF00), 0); // GREEN
    } else if (strcmp(status, Lang::Strings::CONNECTING) == 0 || 
               strcmp(status, Lang::Strings::LOADING_PROTOCOL) == 0) {
        lv_label_set_text(state_icon_label_, FONT_AWESOME_GEAR);
        lv_obj_set_style_text_color(state_icon_label_, lv_color_hex(0xFFFF00), 0); // YELLOW
    } else if (strcmp(status, Lang::Strings::STANDBY) == 0) {
        lv_label_set_text(state_icon_label_, FONT_AWESOME_CLOCK);
        lv_obj_set_style_text_color(state_icon_label_, lv_color_hex(0x0000FF), 0); // BLUE
    } else {
        lv_label_set_text(state_icon_label_, "");
    }
}

void KottyDisplay::ShowNotification(const char* notification, int duration_ms) {
    ESP_LOGI(TAG, "Notification: %s", notification ? notification : "NULL");
}

void KottyDisplay::SetTheme(Theme* theme) {
    // Override to prevent base class from accessing deleted UI elements
    // We only update our specific elements if needed
    DisplayLockGuard lock(this);
    // Kotty currently uses a fixed dark theme (black background), so we don't apply theme changes
    // This empty implementation is sufficient to stop the crash.
}
