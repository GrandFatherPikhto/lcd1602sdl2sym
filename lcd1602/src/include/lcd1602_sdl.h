#ifndef LCD1602_SDL_H
#define LCD1602_SDL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define SDL_LCD_DEFAULT_DEBOUNCE 2

typedef enum {
    LCD_EVENT_NONE,
    LCD_PUSH_BUTTON,
    LCD_LONG_PUSH_BUTTON,
    LCD_DOUBLE_CLICK_BUTTON,
    LCD_CHANGE_POS
} lcd_event_id;

typedef struct lcd_event {
    lcd_event_id event;
    int pos;
} lcd_event_t;

typedef struct sdl_handle sdl_handle_t;

sdl_handle_t *lcd1602_sdl_create(const char *title, int width, int height);
void lcd1602_sdl_release(sdl_handle_t *handle);
bool lcd1602_sdl_next_tick(sdl_handle_t *handle);

void lcd1602_sdl_set_position_cb(sdl_handle_t *handle, void (*cb)(int));
void lcd1602_sdl_set_push_button_cb(sdl_handle_t *handle, void (*cb)(void));
void lcd1602_sdl_set_long_push_button_cb(sdl_handle_t *handle, void (*cb)(void));
void lcd1602_sdl_set_double_click_cb(sdl_handle_t *handle, void (*cb)(void));

void lcd1602_sdl_set_debounce(sdl_handle_t *handle, uint8_t debounce);
void lcd_sdl_clear(sdl_handle_t *handle);
void lcd_sdl_set_cursor(sdl_handle_t *handle, int x, int y);
void lcd_sdl_print_char(sdl_handle_t *handle, char ch);
void lcd_sdl_print_str(sdl_handle_t *handle, const char *str);

#endif /* LCD1602_SDL_H */
