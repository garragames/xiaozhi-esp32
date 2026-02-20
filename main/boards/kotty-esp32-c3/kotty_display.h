#ifndef KOTTY_DISPLAY_H
#define KOTTY_DISPLAY_H

#include "display/lcd_display.h"

class KottyDisplay : public SpiLcdDisplay {
private:
    lv_obj_t* state_icon_label_ = nullptr;
    lv_obj_t* kotty_image_ = nullptr;

protected:
    virtual void SetupUI() override;

public:
    KottyDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                 int width, int height, int offset_x, int offset_y,
                 bool mirror_x, bool mirror_y, bool swap_xy);
    
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetStatus(const char* status) override;
    virtual void ShowNotification(const char* notification, int duration_ms = 3000) override;
    virtual void SetTheme(Theme* theme) override;
};

#endif
