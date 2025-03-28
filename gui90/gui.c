#include "gui.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "text.h"

typedef enum ButtonState {
    BUTTON_UP, BUTTON_CLICKED, BUTTON_DOWN, BUTTON_RELEASED
} ButtonState;

typedef struct Gui90 {
    GUI90_Color* pixels;
    int width;
    int height;
    int mouse_x;
    int mouse_y;
    int current_x;
    int current_y;
    GUI90_Theme theme;
    ButtonState left_mouse_button;
    ButtonState left_arrow_button;
    ButtonState right_arrow_button;
    ButtonState backspace_button;
    ButtonState delete_button;
    ButtonState home_button;
    ButtonState end_button;
    char input_character;
    int active_text_input_widget_index;
    int text_input_widget_index_count;
} Gui90;

// -----------------------------------------------------------------------------
// PRIVATE STUFF

static const int TEXT_SIZE = 8;
static const int BUTTON_TEXT_PADDING = 3;

static Gui90 s_gui;

typedef struct Rectangle {
    int x;
    int y;
    int width;
    int height;
} Rectangle;

// -----------------------------------------------------------------------------
// PRIVATE MOUSE & KEYBOARD FUNCTIONS

static ButtonState updateButtonState(ButtonState old_state, bool is_down) {
    switch (old_state) {
    case BUTTON_UP: return is_down ? BUTTON_CLICKED : BUTTON_UP;
    case BUTTON_CLICKED: return BUTTON_DOWN;
    case BUTTON_DOWN: return is_down ? BUTTON_DOWN : BUTTON_RELEASED;
    case BUTTON_RELEASED: return BUTTON_UP;
    default: return (ButtonState){};
    }
}

static bool isLeftMouseButtonDownInside(Rectangle r) {
    return s_gui.left_mouse_button == BUTTON_DOWN &&
        r.x <= s_gui.mouse_x && s_gui.mouse_x < r.x + r.width &&
        r.y <= s_gui.mouse_y && s_gui.mouse_y < r.y + r.height;
}

static bool isLeftMouseButtonReleasedInside(Rectangle r) {
    return s_gui.left_mouse_button == BUTTON_RELEASED &&
        r.x <= s_gui.mouse_x && s_gui.mouse_x < r.x + r.width &&
        r.y <= s_gui.mouse_y && s_gui.mouse_y < r.y + r.height;
}

// -----------------------------------------------------------------------------
// PRIVATE DRAW FUNCTIONS

static void drawPoint(int x, int y, GUI90_Color color) {
    s_gui.pixels[y * s_gui.width + x] = color;
}

static void drawRectangle(Rectangle rectangle, GUI90_Color color) {
    for (auto dy = 0; dy < rectangle.height; ++dy) {
        for (auto dx = 0; dx < rectangle.width; ++dx) {
            drawPoint(rectangle.x + dx, rectangle.y + dy, color);
        }
    }
}

static void drawLineHorizontal(int x, int y, int width, GUI90_Color color) {
    auto r = (Rectangle){};
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = 1;
    drawRectangle(r, color);
}

static void drawLineVertical(int x, int y, int height, GUI90_Color color) {
    auto r = (Rectangle){};
    r.x = x;
    r.y = y;
    r.width = 1;
    r.height = height;
    drawRectangle(r, color);
}

GUI90_SunkenFrame GUI90_UpdateSunkenFrame(GUI90_SunkenFrame widget) {
    auto x = widget.x;
    auto y = widget.y;
    auto width = widget.width;
    auto height = widget.height;
    auto rectangle = (Rectangle){x + 1, y + 1, width - 2, height - 2};
    drawRectangle(rectangle, s_gui.theme.recess_background);
    drawLineHorizontal(x + 1, y, width - 2, s_gui.theme.recess_bevel_dark);
    drawLineHorizontal(x + 1, y + height - 1, width - 2, s_gui.theme.recess_bevel_light);
    drawLineVertical(x, y + 1, height - 2, s_gui.theme.recess_bevel_dark);
    drawLineVertical(x + width - 1, y + 1, height - 2, s_gui.theme.recess_bevel_light);
    widget.is_clicked = isLeftMouseButtonReleasedInside(rectangle);
    return widget;
}

static void drawCursor(size_t x_start, size_t y_start, GUI90_Color color) {
    for (size_t y = 0; y < 8; ++y) {
        s_gui.pixels[(y_start + y) * s_gui.width + x_start] = color;
    }
}

static void drawCharacter(char character, size_t x_start, size_t y_start, GUI90_Color color) {
    auto W = s_gui.width;
    auto character_bitmap = character_bitmap8x8(character);
    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            if (character_bitmap[y * 8 + x]) {
                s_gui.pixels[(y_start + y) * W + x_start + x] = color;
            }
        }
    }
}

static void drawString(const char* s, size_t x, size_t y, GUI90_Color color) {
    for (; *s; ++s, x += 8) {
        drawCharacter(*s, x, y, color);
    }
}

static void drawSpecialString(const char* s, int x, int y, GUI90_HeaderLabelTheme theme) {
    for (; *s; ++s, x += 8) {
        if (theme.draw_up_left) {
            drawCharacter(*s, x - 1, y - 1, theme.color_up_left);
        }
        if (theme.draw_up_right) {
            drawCharacter(*s, x + 1, y - 1, theme.color_up_right);
        }
        if (theme.draw_down_left) {
            drawCharacter(*s, x - 1, y + 1, theme.color_down_left);
        }
        if (theme.draw_down_right) {
            drawCharacter(*s, x + 1, y + 1, theme.color_down_right);
        }
        
        if (theme.draw_up) {
            drawCharacter(*s, x + 0, y - 1, theme.color_up);
        }
        if (theme.draw_left) {
            drawCharacter(*s, x - 1, y + 0, theme.color_left);
        }
        if (theme.draw_right) {
            drawCharacter(*s, x + 1, y + 0, theme.color_right);
        }
        if (theme.draw_down) {
            drawCharacter(*s, x + 0, y + 1, theme.color_down);
        }
        
        if (theme.draw_center) {
            drawCharacter(*s, x + 0, y + 0, theme.color_center);
        }
    }
}

// -----------------------------------------------------------------------------
// PUBLIC FUNCTIONS

void GUI90_Init(int width, int height) {
    if (s_gui.pixels != NULL) {
        free(s_gui.pixels);
    }
    s_gui = (Gui90){};
    s_gui.pixels = (GUI90_Color *)malloc(width * height * sizeof(GUI90_Color));
    s_gui.width = s_gui.pixels ? width : 0;
    s_gui.height = s_gui.pixels ? height : 0;
    s_gui.theme = GUI90_THEME_GRAY;
    s_gui.active_text_input_widget_index = -1;
}

void GUI90_SetInput(GUI90_Input input) {
    // Mouse:
    s_gui.mouse_x = input.mouse_x;
    s_gui.mouse_y = input.mouse_y;
    s_gui.left_mouse_button = updateButtonState(s_gui.left_mouse_button, input.is_left_mouse_button_down);
    // Keyboard:
    s_gui.left_arrow_button = updateButtonState(s_gui.left_arrow_button, input.is_left_arrow_button_down);
    s_gui.right_arrow_button = updateButtonState(s_gui.right_arrow_button, input.is_right_arrow_button_down);
    s_gui.backspace_button = updateButtonState(s_gui.backspace_button, input.is_backspace_button_down);
    s_gui.delete_button = updateButtonState(s_gui.delete_button, input.is_delete_button_down);
    s_gui.home_button = updateButtonState(s_gui.home_button, input.is_home_button_down);
    s_gui.end_button = updateButtonState(s_gui.end_button, input.is_end_button_down);
    s_gui.input_character = input.input_character;
    // Other:
    s_gui.text_input_widget_index_count = 0;
}

void GUI90_SetTheme(GUI90_Theme theme) {
    s_gui.theme = theme;
}

const GUI90_Color* GUI90_GetPixelData() {
    return s_gui.pixels;
}

void GUI90_WidgetBackground() {
    auto pixel_count = s_gui.width * s_gui.height;
    for (int i = 0; i < pixel_count; ++i) {
        s_gui.pixels[i] = s_gui.theme.background; 
    }
}

static Rectangle textRectangle(int x, int y, const char* text) {
    return (Rectangle){
        .x = x,
        .y = y,
        .width = 8 * (int)(strlen(text)),
        .height = 8,    
    };
}

GUI90_Label GUI90_UpdateLabel(GUI90_Label widget) {
    drawString(widget.text, widget.x, widget.y, s_gui.theme.text);
    auto rectangle = textRectangle(widget.x, widget.y, widget.text);
    widget.width = rectangle.width;
    widget.height = rectangle.height;
    widget.is_clicked = isLeftMouseButtonReleasedInside(rectangle);
    return widget;
}

GUI90_HeaderLabel GUI90_UpdateHeaderLabel(GUI90_HeaderLabel widget) {
    drawSpecialString(widget.text, widget.x, widget.y, widget.theme);
    auto rectangle = textRectangle(widget.x, widget.y, widget.text);
    widget.width = rectangle.width;
    widget.height = rectangle.height;
    widget.is_clicked = isLeftMouseButtonReleasedInside(rectangle);
    return widget;
}

GUI90_Button GUI90_WidgetButtonBevel(GUI90_Button widget) {
    auto x = widget.x;
    auto y = widget.y;
    x += 1;
    y += 1;
    auto rectangle = (Rectangle){};
    rectangle.x = x;
    rectangle.y = y;
    rectangle.width = 8 * (int)(strlen(widget.text)) + 2 * BUTTON_TEXT_PADDING;
    rectangle.height = 8 + 2 * BUTTON_TEXT_PADDING;
    auto inner_rectangle = rectangle;
    inner_rectangle.x += 2;
    inner_rectangle.y += 2;
    inner_rectangle.width -= 4;
    inner_rectangle.height -= 4;
    auto text_x = x + BUTTON_TEXT_PADDING;
    auto text_y = y + BUTTON_TEXT_PADDING;
    auto theme = s_gui.theme;
    if (isLeftMouseButtonDownInside(rectangle)) {
        ++text_y;
        theme.button_bevel_light = s_gui.theme.button_background;
        theme.button_bevel_dark = s_gui.theme.button_background;
    }
    
    drawRectangle(rectangle, s_gui.theme.button_border);
    
    // Rounded corners:
    // drawLineHorizontal(rectangle.x + 1, rectangle.y, rectangle.width - 2, theme.button_border);
    // drawLineHorizontal(rectangle.x + 1, rectangle.y + rectangle.height - 1, rectangle.width - 2, theme.button_border);
    // drawLineVertical(rectangle.x, rectangle.y + 1, rectangle.height - 2, theme.button_border);
    // drawLineVertical(rectangle.x + rectangle.width - 1, rectangle.y + 1, rectangle.height - 2, theme.button_border);
    // drawPoint(rectangle.x + 1, rectangle.y + 1, theme.button_border);
    // drawPoint(rectangle.x + 1, rectangle.y + rectangle.height - 2, theme.button_border);
    // drawPoint(rectangle.x + + rectangle.width - 2, rectangle.y + 1, theme.button_border);
    // drawPoint(rectangle.x + + rectangle.width - 2, rectangle.y + rectangle.height - 2, theme.button_border);
    
    drawRectangle(inner_rectangle, theme.button_background);
    drawLineHorizontal(x + 2, y + 1, rectangle.width - 4, theme.button_bevel_light);
    drawLineHorizontal(x + 2, y + rectangle.height - 2, rectangle.width - 4, theme.button_bevel_dark);
    drawLineVertical(x + 1, y + 2, rectangle.height - 4, theme.button_bevel_light);
    drawLineVertical(x + rectangle.width - 2, y + 2, rectangle.height - 4, theme.button_bevel_dark);
    drawString(widget.text, text_x, text_y, theme.text);

    widget.width = rectangle.width + 2;
    widget.height = rectangle.height + 2;
    widget.is_clicked = isLeftMouseButtonReleasedInside(rectangle);
    return widget;
}

GUI90_Button GUI90_WidgetButtonCloud(GUI90_Button widget) {
    auto x = widget.x;
    auto y = widget.y;
    x += 1;
    y += 1;
    auto rectangle = (Rectangle){};
    rectangle.x = x;
    rectangle.y = y;
    rectangle.width = 8 * (int)(strlen(widget.text)) + 2 * BUTTON_TEXT_PADDING;
    rectangle.height = 8 + 2 * BUTTON_TEXT_PADDING - 1;

    if (isLeftMouseButtonDownInside(rectangle)) {
        rectangle.y += 1;
    }
    
    auto inner_rectangle = rectangle;
    inner_rectangle.x += 1;
    inner_rectangle.y += 1;
    inner_rectangle.width -= 2;
    inner_rectangle.height -= 2;
    auto text_x = x + BUTTON_TEXT_PADDING;
    auto text_y = y + BUTTON_TEXT_PADDING - 1;
    auto theme = s_gui.theme;
    if (isLeftMouseButtonDownInside(rectangle)) {
        ++text_y;
    }

    // Shadow:
    drawPoint(x, y + rectangle.height - 1, theme.button_border);
    drawPoint(x + rectangle.width - 1, y + rectangle.height - 1, theme.button_border);
    drawLineHorizontal(x + 1, y + rectangle.height, rectangle.width - 2, theme.button_border);
    
    //drawRectangle(rectangle, s_gui.theme.button_border);
    drawRectangle(inner_rectangle, theme.button_background);
    
    // Rounded corners:
    drawLineHorizontal(rectangle.x + 1, rectangle.y, rectangle.width - 2, theme.button_border);
    drawLineHorizontal(rectangle.x + 1, rectangle.y + rectangle.height - 1, rectangle.width - 2, theme.button_border);
    drawLineVertical(rectangle.x, rectangle.y + 1, rectangle.height - 2, theme.button_border);
    drawLineVertical(rectangle.x + rectangle.width - 1, rectangle.y + 1, rectangle.height - 2, theme.button_border);
    drawPoint(rectangle.x + 1, rectangle.y + 1, theme.button_border);
    drawPoint(rectangle.x + 1, rectangle.y + rectangle.height - 2, theme.button_border);
    drawPoint(rectangle.x + + rectangle.width - 2, rectangle.y + 1, theme.button_border);
    drawPoint(rectangle.x + + rectangle.width - 2, rectangle.y + rectangle.height - 2, theme.button_border);
    
    drawPoint(rectangle.x + 2, rectangle.y + 1, theme.button_bevel_dark);
    drawPoint(rectangle.x + 1, rectangle.y + 2, theme.button_bevel_dark);
    drawPoint(rectangle.x + 1, rectangle.y + rectangle.height - 3, theme.button_bevel_dark);
    drawPoint(rectangle.x + 2, rectangle.y + rectangle.height - 2, theme.button_bevel_dark);
    drawPoint(rectangle.x + rectangle.width - 3, rectangle.y + 1, theme.button_bevel_dark);
    drawPoint(rectangle.x + rectangle.width - 2, rectangle.y + 2, theme.button_bevel_dark);
    drawPoint(rectangle.x + rectangle.width - 3, rectangle.y + rectangle.height - 2, theme.button_bevel_dark);
    drawPoint(rectangle.x + rectangle.width - 2, rectangle.y + rectangle.height - 3, theme.button_bevel_dark);
    
    drawString(widget.text, text_x, text_y, theme.text);
    widget.width = rectangle.width + 2;
    widget.height = 2 * GUI90_BLOCK;
    widget.is_clicked = isLeftMouseButtonReleasedInside(rectangle);
    return widget;
}

GUI90_Button GUI90_UpdateButton(GUI90_Button widget) {
    switch (s_gui.theme.button_type) {
        case BUTTON_TYPE_BEVEL: return GUI90_WidgetButtonBevel(widget);
        case BUTTON_TYPE_CLOUD: return GUI90_WidgetButtonCloud(widget);
        default: return GUI90_WidgetButtonBevel(widget);
    }
}

GUI90_RadioButton GUI90_UpdateRadioButton(GUI90_RadioButton widget) {
    for (int yi = 0; yi < 16; ++yi) {
        for (int xi = 0; xi < 16; ++xi) {
            double dx = xi - 7.5;
            double dy = yi - 7.5;
            double r2 = dx * dx + dy * dy;
            auto color = s_gui.theme.background;
            if (r2 < 6.0 * 6.0 && dx + dy < 0.0) {
                color = s_gui.theme.recess_bevel_dark;
            }
            if (r2 < 6.0 * 6.0 && dx + dy > 0.0) {
                color = s_gui.theme.recess_bevel_light;
            }
            if (r2 < 5.0 * 5.0) {
                color = s_gui.theme.recess_background;
            }
            if (r2 < 2.0 * 2.0 && widget.is_selected) {
                color = s_gui.theme.recess_text_selected;
            }
            drawPoint(widget.x + xi, widget.y + yi, color);
        }
    }
    auto left_rectangle = (Rectangle){widget.x, widget.y, 16 + BUTTON_TEXT_PADDING, 16};
    auto label = (GUI90_Label){
        .x=widget.x + 16 + BUTTON_TEXT_PADDING,
        .y=widget.y + BUTTON_TEXT_PADDING,
        .text=widget.text
    };
    auto label_result = GUI90_UpdateLabel(label);
    widget.width = label_result.width + 16 + 8;
    widget.height = 16;
    widget.is_clicked = label_result.is_clicked || isLeftMouseButtonReleasedInside(left_rectangle);
    return widget;
}

GUI90_Stepper GUI90_UpdateStepper(GUI90_Stepper widget) {
    auto x = widget.x;
    auto y = widget.y;
    auto offset = 0;
    auto label = (GUI90_Label){
        .x=x + offset,
        .y=y + BUTTON_TEXT_PADDING,
        .text=widget.text
    };
    auto label_widget = GUI90_UpdateLabel(label);
    offset += label_widget.width;
    auto decrease_button = (GUI90_Button){.x=x + offset, .y=y, .text="-"};
    decrease_button = GUI90_UpdateButton(decrease_button);
    offset += decrease_button.width;
    auto increase_button = (GUI90_Button){.x=x + offset, .y=y, .text="+"};
    increase_button = GUI90_UpdateButton(increase_button);
    offset += increase_button.width;

    widget.width = offset;
    widget.height = increase_button.height;
    widget.is_clicked = increase_button.is_clicked || decrease_button.is_clicked;
    widget.is_increased = increase_button.is_clicked;
    widget.is_decreased = decrease_button.is_clicked;
    return widget;
}

GUI90_SelectionBoxInit GUI90_UpdateSelectionBoxInit(GUI90_SelectionBoxInit widget) {
    s_gui.current_x = widget.x + TEXT_SIZE;
    s_gui.current_y = widget.y + TEXT_SIZE;
    auto frame = (GUI90_SunkenFrame){.x=widget.x, .y=widget.y, .width=widget.width, .height=widget.height};
    frame = GUI90_UpdateSunkenFrame(frame);
    return widget;
}

GUI90_SelectionBoxItem GUI90_UpdateSelectionBoxItem(GUI90_SelectionBoxItem widget) {
    auto global_theme = s_gui.theme;
    auto local_theme = s_gui.theme;
    local_theme.text = widget.is_selected ? local_theme.recess_text_selected : local_theme.recess_text;
    GUI90_SetTheme(local_theme);
    auto label = (GUI90_Label){
        .x=s_gui.current_x,
        .y=s_gui.current_y,
        .text=widget.text
    };
    label = GUI90_UpdateLabel(label);
    s_gui.current_y += label.height;
    GUI90_SetTheme(global_theme);
    widget.width = label.width;
    widget.height = label.height;
    widget.is_clicked = label.is_clicked;
    return widget;
}

static GUI90_TextInput decrementCursor(GUI90_TextInput widget) {
    if (widget.cursor > 0) {
        widget.cursor--;
    }
    return widget;
}

static GUI90_TextInput incrementCursor(GUI90_TextInput widget) {
    if (widget.cursor < strlen(widget.text)) {
        widget.cursor++;
    }
    return widget;
}

static void deleteCharacter(char* string, size_t index) {
    auto len = strlen(string);
    for (size_t i = index; i < len; ++i) {
        string[i] = string[i + 1];
    }
}

static void insertCharacter(char* string, size_t index, char c) {
    size_t len = strlen(string);
    size_t max_size = GUI90_MAX_SINGLE_LINE_TEXT_INPUT;
    if (len + 1 >= max_size || index > len) {
        return;
    }
    for (size_t i = len + 1; i > index; i--) {
        string[i] = string[i - 1];
    }
    string[index] = c;
}


GUI90_TextInput GUI90_UpdateTextInput(GUI90_TextInput widget) {
    auto widget_index = s_gui.text_input_widget_index_count++;
    auto is_selected = s_gui.active_text_input_widget_index == widget_index;
    if (is_selected) {
        if (s_gui.home_button == BUTTON_CLICKED) {
            widget.cursor = 0;
        }
        if (s_gui.end_button == BUTTON_CLICKED) {
            widget.cursor = strlen(widget.text);
        }
        if (s_gui.left_arrow_button == BUTTON_CLICKED) {
            widget = decrementCursor(widget);
        }
        if (s_gui.right_arrow_button == BUTTON_CLICKED) {
            widget = incrementCursor(widget);
        }
        if (s_gui.input_character && strlen(widget.text) < GUI90_MAX_SINGLE_LINE_TEXT_INPUT - 1) {
            insertCharacter(widget.text, widget.cursor, s_gui.input_character);
            widget = incrementCursor(widget);
        }
        if (s_gui.delete_button == BUTTON_CLICKED) {
            deleteCharacter(widget.text, widget.cursor);
        }
        if (s_gui.backspace_button == BUTTON_CLICKED && widget.cursor > 0) {
            deleteCharacter(widget.text, widget.cursor - 1);
            widget = decrementCursor(widget);
        }
    }

    auto x = widget.x;
    auto y = widget.y;
    auto width = 8 * (GUI90_MAX_SINGLE_LINE_TEXT_INPUT - 1) + GUI90_BLOCK;
    auto height = 8 + GUI90_BLOCK;
    widget.width = width;
    widget.height = height;

    auto frame = (GUI90_SunkenFrame){.x=x, .y=y, .width=width, .height=height};
    frame = GUI90_UpdateSunkenFrame(frame);

    widget.is_clicked = frame.is_clicked;
    if (widget.is_clicked) {
        s_gui.active_text_input_widget_index = widget_index;
    }

    auto global_theme = s_gui.theme;
    auto local_theme = s_gui.theme;
    local_theme.text = is_selected ? local_theme.recess_text_selected : local_theme.recess_text;
    GUI90_SetTheme(local_theme);
    drawString(widget.text, x + GUI90_BLOCK / 2, y + GUI90_BLOCK / 2, s_gui.theme.text);
    if (is_selected) {
        auto cursor_x = x + GUI90_BLOCK / 2 + widget.cursor * TEXT_SIZE;
        auto cursor_y = y + GUI90_BLOCK / 2;
        drawCursor(cursor_x, cursor_y, s_gui.theme.text);
    }
    GUI90_SetTheme(global_theme);

    return widget;
}
