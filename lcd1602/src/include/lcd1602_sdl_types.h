#ifndef LCD1602_SDL_TYPES_H
#define LCD1602_SDL_TYPES_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LCD_EVENT_QUEUE_SIZE 0x40
#define MAX_LCD_TITLE 0x40
#define MAX_FONT_PATH 0x100

typedef enum {
    EVT_NONE = 0,
    EVT_KEY_DOWN,
    EVT_KEY_UP,
    EVT_MOUSE_BUTTON_DOWN,
    EVT_MOUSE_BUTTON_UP,
    EVT_MOUSE_MOTION,
    EVT_QUIT
} sdl_event_type_t;

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

typedef struct {
    sdl_event_type_t type;
    union {
        struct {
            SDL_Keycode key;
            uint16_t scancode;
            uint8_t mod;
        } key;
        struct {
            uint8_t button; // SDL_BUTTON_LEFT/RIGHT/...
            int x, y;
        } mouse_btn;
        struct {
            int x;
            int y;
            int xrel;
            int yrel;
            uint32_t buttons;
        } mouse_motion;
    } data;
} sdl_event_t;

typedef struct sdl_event_queue {
    sdl_event_t buffer[LCD_EVENT_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    SDL_mutex *mutex;
    SDL_cond *cond_not_empty;
    SDL_cond *cond_not_full;
} sdl_event_queue_t;

typedef struct lcd1602_handle {
    char display[2][17]; 
    int cursor_x; 
    int cursor_y; 
    bool backlight; 
    bool blink; 
    int contrast; 
    int pos;
    uint8_t debounce;
} lcd1602_handle_t;

typedef struct thread_context {
    SDL_Window *win;
    sdl_event_queue_t *queue;
    SDL_mutex *mutex;
    SDL_cond *cond;
    bool ended;
    bool *running;
} thread_context_t;

typedef struct sdl_handle {
    const char title[MAX_LCD_TITLE];
    int width; 
    int height; 

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Thread* thread;
    char fontPath[MAX_FONT_PATH];
    TTF_Font *font;

    lcd1602_handle_t *lcd;
    sdl_event_queue_t *queue;
    thread_context_t *thread_ctx;

    void (*position_cb)(int);
    void (*push_button_cb)(void);
    void (*long_push_button_cb)(void);
    void (*double_click_cb)(void);

    bool running;
    
    bool dirty;
} sdl_handle_t;

#endif // LCD1602_SDL_TYPES_H