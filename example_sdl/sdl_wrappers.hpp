#pragma once

#include <SDL2/SDL.h>

enum ButtonState {BUTTON_UP, BUTTON_CLICKED, BUTTON_DOWN, BUTTON_RELEASED};

struct Input {
    int mouse_x;
    int mouse_y;
    int mouse_dx;
    int mouse_dy;
    Uint32 mouse;
    const Uint8* keyboard;
    bool isLeftMouseButtonDown() const;
    bool isRightMouseButtonDown() const;
    bool isLeftMouseButtonReleased() const;
    ButtonState left_mouse_button;
    ButtonState escape_button;
};

class Sdl
{
public:
    Sdl(const char* window_title, int width, int height);
    ~Sdl();
    static bool noQuitMessage();
    Input getInput();
    void setMouseModeRelative();
    void setMouseModeAbsolute();
    void draw(const Uint32* pixels);
    int width;
    int height;
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    ButtonState left_mouse_button;
    ButtonState escape_button;
};
