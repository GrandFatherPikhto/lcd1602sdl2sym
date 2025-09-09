#ifndef LCD1602_SDL_H
#define LCD1602_SDL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    LCD_CMD_CLEAR,
    LCD_CMD_SET_CURSOR,
    LCD_CMD_PRINT_CHAR,
    LCD_CMD_PRINT_STR,
    LCD_CMD_KEY_LEFT,
    LCD_CMD_KEY_RIGHT,
    LCD_CMD_KEY_UP,
    LCD_CMD_KEY_DOWN,
    LCD_CMD_KEY_ENTER,
    LCD_CMD_QUIT,
} lcd_command_t;

typedef struct lcd1602_handle lcd1602_handle_t;
typedef struct sdl_handle sdl_handle_t;
bool lcd1602_sdl_next_tick(sdl_handle_t *handle);

sdl_handle_t *lcd1602_sdl_create(const char *title, int width, int height);
void lcd1602_sdl_release(sdl_handle_t *handle);

#endif // LCD1602_SDL_H