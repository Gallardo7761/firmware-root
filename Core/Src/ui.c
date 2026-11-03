/**
 * @file    ui.c
 * @author  Jose
 * @brief   Mini UI framework for SH1106 screen using STM32
 * @date    20/7/26
 * @project firmware-root
 */

#include "ui.h"
#include "sh1106.h"
#include <stdint.h>

#define UI_RETURN_IF_INVALID(c) do { if ((c) == NULL) return; } while (0)

const SH1106_Font_t Font_10x16 = {
    .height = 16,
    .width = 10,
    .map = Map_10x16,
    .size = SH1106_FONT_10x16
  };

const SH1106_Font_t Font_5x8 = {
    .height = 8,
    .width = 5,
    .map = Map_5x8,
    .size = SH1106_FONT_5x8
  };

void UI_Container_Split_H(const UI_Container_t *parent, UI_Container_t *left, UI_Container_t *right, uint8_t split_pct) 
{
    uint8_t left_w = (parent->w * split_pct) / 100;
    
    *left = (UI_Container_t) {
        parent->x,
        parent->y,
        left_w,
        parent->h,
        parent->padding
    };

    *right = (UI_Container_t) {
        parent->x + left_w,
        parent->y,
        parent->w - left_w,
        parent->h,
        parent->padding
    };
}

void UI_Container_Split_V(const UI_Container_t *parent, UI_Container_t *top, UI_Container_t *bottom, uint8_t split_pct) 
{
    uint8_t top_h = (parent->h * split_pct) / 100;
    
    *top = (UI_Container_t) {
        parent->x,
        parent->y,
        parent->w,
        top_h,
        parent->padding
    };
    
    *bottom = (UI_Container_t) {
        parent->x,
        parent->y + top_h,
        parent->w,
        parent->h - top_h,
        parent->padding
    };
}

void UI_Container_Draw_Text(SH1106_Handle_t *h, const UI_Container_t *c, uint8_t rel_x, uint8_t rel_y, SH1106_Pixel_State_t state, const SH1106_Font_t *font, const char *format, ...)
{
    UI_RETURN_IF_INVALID(c);

    char buffer[32];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    uint8_t abs_x = c->x + c->padding + rel_x;
    uint8_t abs_y = c->y + c->padding + rel_y;

    if ((abs_x < (c->x + c->w)) && (abs_y < (c->y + c->h))) {
        SH1106_Draw_Text(h, abs_x, abs_y, state, font, buffer);
    }
}

void UI_Container_Draw_Button(SH1106_Handle_t *h, const UI_Container_t *c, uint8_t rel_x, uint8_t rel_y, uint8_t width, uint8_t height, const char* label, bool is_focused)
{
    UI_RETURN_IF_INVALID(c);

    uint8_t abs_x = c->x + c->padding + rel_x;
    uint8_t abs_y = c->y + c->padding + rel_y;
    
    uint8_t text_width = strlen(label) * 11;
    uint8_t text_x = abs_x + ((width - text_width) / 2);
    uint8_t text_y = abs_y + ((height - 16) / 2);

    if (is_focused) {
        for (uint8_t i = abs_y; i < abs_y + height; i++) {
            for (uint8_t j = abs_x; j < abs_x + width; j++) {
                SH1106_Draw_Pixel(h, j, i, SH1106_PIXEL_ON);
            }
        }
        SH1106_Draw_Text(h, text_x, text_y, SH1106_PIXEL_OFF, &Font_10x16, label);
    } else {
        SH1106_Draw_Rectangle(h, abs_x, abs_y, width, height, SH1106_PIXEL_ON);
        SH1106_Draw_Text(h, text_x, text_y, SH1106_PIXEL_ON, &Font_10x16, label);
    }
}

void UI_Container_Draw_Icon(SH1106_Handle_t *h, const UI_Container_t *c, uint8_t rel_x, uint8_t rel_y, UI_IconIndex_t icon, SH1106_Pixel_State_t state)
{
    UI_RETURN_IF_INVALID(c);

    uint8_t abs_x = c->x + c->padding + rel_x;
    uint8_t abs_y = c->y + c->padding + rel_y;

    const uint8_t *bitmap_ptr = NULL;
    uint8_t size = 0;

    if (icon >= ICON_7x7_BATTERY_EMPTY) {
        bitmap_ptr = UI_Icons7x7[icon - ICON_7x7_BATTERY_EMPTY];
        size = 7;
    } else {
        bitmap_ptr = UI_Icons15x15[icon];
        size = 15;
    }

    if ((abs_x + size <= c->x + c->w) && (abs_y + size <= c->y + c->h)) {
        SH1106_Draw_Bitmap(h, abs_x, abs_y, bitmap_ptr, size, size, state);
    }
}

void UI_Draw_Status_Bar(SH1106_Handle_t *h, const UI_Container_t *c, const char *title, uint8_t battery_level, bool connected)
{
    UI_RETURN_IF_INVALID(c);

    for (uint8_t y = c->y; y < (c->y + c->h); y++) {
        for (uint8_t x = c->x; x < (c->x + c->w); x++) {
            SH1106_Draw_Pixel(h, x, y, SH1106_PIXEL_ON);
        }
    }

    UI_Container_Draw_Text(h, c, 0, 1, SH1106_PIXEL_OFF, &Font_5x8, title);

    uint8_t battery_rel_x = c->w - 7 - (c->padding * 2);
    uint8_t net_rel_x = c->w - 21;

    if (battery_level < 10) {
        UI_Container_Draw_Icon(h, c, battery_rel_x, 1, ICON_7x7_BATTERY_EMPTY, SH1106_PIXEL_OFF);
    } else if (battery_level >= 10 && battery_level < 25) {
        UI_Container_Draw_Icon(h, c, battery_rel_x, 1, ICON_7x7_BATTERY_LOW, SH1106_PIXEL_OFF);
    } else if (battery_level >= 25 && battery_level < 50) {
        UI_Container_Draw_Icon(h, c, battery_rel_x, 1, ICON_7x7_BATTERY_HALF, SH1106_PIXEL_OFF);
    } else if (battery_level >= 50 && battery_level < 75) {
        UI_Container_Draw_Icon(h, c, battery_rel_x, 1, ICON_7x7_BATTERY_ALMOST_FULL, SH1106_PIXEL_OFF);
    } else if (battery_level >= 75 && battery_level <= 100) {
        UI_Container_Draw_Icon(h, c, battery_rel_x, 1, ICON_7x7_BATTERY_FULL, SH1106_PIXEL_OFF);
    }

    if (connected) {
        UI_Container_Draw_Icon(h, c, net_rel_x, 1, ICON_7x7_NET_CONNECTED, SH1106_PIXEL_OFF);
    } else {
        UI_Container_Draw_Icon(h, c, net_rel_x, 1, ICON_7x7_NET_DISCONNECTED, SH1106_PIXEL_OFF);
    }
}