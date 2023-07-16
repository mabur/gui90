#pragma once

#include <cstdint>

using Color = uint32_t;

struct ColorShades {
    Color foreground;
    Color background;
    Color bevel_light;
    Color bevel_dark;
    Color border;
};

extern const ColorShades GUI90_YELLOW_SHADES;
extern const ColorShades GUI90_GRAY_SHADES;
extern const ColorShades GUI90_LEATHER_SHADES;

enum class ButtonState {UP, CLICKED, DOWN, RELEASED};

struct Gui90;

Gui90* GUI90_Init(int width, int height);
void GUI90_Destroy(Gui90* gui);
void GUI90_SetMouseState(Gui90* gui, int x, int y, ButtonState left_mouse_button);
const Color* GUI90_GetPixelData(const Gui90* gui);

void GUI90_WidgetBackground(Gui90* gui, ColorShades shades);
bool GUI90_WidgetLabel(Gui90* gui, int x, int y, const char* text, ColorShades shades);
bool GUI90_WidgetButton(Gui90* gui, int x, int y, const char* text, ColorShades shades);
void GUI90_WidgetIntSetting(Gui90* gui, int x, int y, const char* text, int* value, ColorShades label_shades, ColorShades button_shades);
