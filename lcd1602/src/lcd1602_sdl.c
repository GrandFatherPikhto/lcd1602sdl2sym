#include "lcd1602_sdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LCD_TITLE 64
#define MAX_FONT_PATH 256

typedef struct {
    char display[2][17];
    int cursor_x, cursor_y;
    bool backlight, blink;
    int contrast, pos;
    uint8_t debounce;
} lcd1602_handle_t;

typedef struct sdl_handle {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    TTF_Font *small_font;
    lcd1602_handle_t *lcd;
    
    void (*position_cb)(int);
    void (*push_button_cb)(void);
    void (*long_push_button_cb)(void);
    void (*double_click_cb)(void);
    
    bool running;
    char font_path[MAX_FONT_PATH];
} sdl_handle_t;

static void s_sdl_render(sdl_handle_t *handle);
static void s_handle_sdl_event(sdl_handle_t *handle, SDL_Event *e);

sdl_handle_t *lcd1602_sdl_create(const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return NULL;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return NULL;
    }

    sdl_handle_t *handle = malloc(sizeof(sdl_handle_t));
    if (!handle) return NULL;
    memset(handle, 0, sizeof(sdl_handle_t));

    handle->lcd = malloc(sizeof(lcd1602_handle_t));
    if (!handle->lcd) {
        free(handle);
        return NULL;
    }
    memset(handle->lcd, 0, sizeof(lcd1602_handle_t));
    
    // Инициализация дисплея
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            handle->lcd->display[y][x] = ' ';
        }
        handle->lcd->display[y][16] = '\0';
    }
    handle->lcd->debounce = SDL_LCD_DEFAULT_DEBOUNCE;
    handle->running = true;

    // Создание окна
    handle->window = SDL_CreateWindow(title ? title : "LCD1602 Simulator",
                                     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                     360, 150, SDL_WINDOW_SHOWN);
    if (!handle->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        free(handle->lcd);
        free(handle);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    handle->renderer = SDL_CreateRenderer(handle->window, -1, SDL_RENDERER_ACCELERATED);
    if (!handle->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(handle->window);
        free(handle->lcd);
        free(handle);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    // Загрузка шрифтов
    handle->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20);
    if (!handle->font) {
        handle->font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 20);
        if (!handle->font) {
            printf("Font loading failed, using default\n");
        }
    }

    handle->small_font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 12);
    if (!handle->small_font) {
        handle->small_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 12);
        if (!handle->small_font) {
            printf("Small font loading failed, using default\n");
        }
    }

    return handle;
}

bool lcd1602_sdl_next_tick(sdl_handle_t *handle) {
    if (!handle) return false;
    
    // Обработка событий SDL в главном потоке
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        s_handle_sdl_event(handle, &e);
    }
    
    // Рендеринг
    s_sdl_render(handle);
    
    return handle->running;
}

static void s_handle_sdl_event(sdl_handle_t *handle, SDL_Event *e) {
    switch (e->type) {
        case SDL_QUIT:
            handle->running = false;
            break;
            
        case SDL_KEYUP:
            switch (e->key.keysym.sym) {
                case SDLK_q:
                    handle->running = false;
                    break;
                case SDLK_RETURN:
                    if (handle->push_button_cb) handle->push_button_cb();
                    break;
                case SDLK_d:
                    if (handle->double_click_cb) handle->double_click_cb();
                    break;
                case SDLK_l:
                    if (handle->long_push_button_cb) handle->long_push_button_cb();
                    break;
                case SDLK_UP:
                    if (handle->position_cb) {
                        handle->lcd->pos += handle->lcd->debounce;
                        handle->position_cb(handle->lcd->pos);
                    }
                    break;
                case SDLK_DOWN:
                    if (handle->position_cb) {
                        handle->lcd->pos -= handle->lcd->debounce;
                        handle->position_cb(handle->lcd->pos);
                    }
                    break;
            }
            break;
    }
}

static void s_sdl_render_hint(sdl_handle_t *handle) {
    if (handle->small_font) {
        SDL_Color hint_color = {150, 150, 150, 255};
        const char *hint_line1 = "Enter:Click L:Long D:Double Q:Exit";
        const char *hint_line2 = "Up/Down:Rotate";

        SDL_Surface *surface1 = TTF_RenderText_Solid(handle->small_font, hint_line1, hint_color);
        if (surface1) {
            SDL_Texture *texture1 = SDL_CreateTextureFromSurface(handle->renderer, surface1);
            SDL_Rect rect1 = {20, 110, surface1->w, surface1->h};
            SDL_RenderCopy(handle->renderer, texture1, NULL, &rect1);
            SDL_FreeSurface(surface1);
            SDL_DestroyTexture(texture1);
        }

        SDL_Surface *surface2 = TTF_RenderText_Solid(handle->small_font, hint_line2, hint_color);
        if (surface2) {
            SDL_Texture *texture2 = SDL_CreateTextureFromSurface(handle->renderer, surface2);
            SDL_Rect rect2 = {20, 125, surface2->w, surface2->h};
            SDL_RenderCopy(handle->renderer, texture2, NULL, &rect2);
            SDL_FreeSurface(surface2);
            SDL_DestroyTexture(texture2);
        }
    }
}

static void s_sdl_render(sdl_handle_t *handle) {
    // Очистка
    SDL_SetRenderDrawColor(handle->renderer, 40, 40, 40, 255);
    SDL_RenderClear(handle->renderer);
    
    // Дисплей LCD
    SDL_Rect lcd_rect = {20, 20, 320, 80};
    SDL_SetRenderDrawColor(handle->renderer, 0, 80, 0, 255);
    SDL_RenderFillRect(handle->renderer, &lcd_rect);
    SDL_SetRenderDrawColor(handle->renderer, 0, 60, 0, 255);
    SDL_RenderDrawRect(handle->renderer, &lcd_rect);
    
    // Текст
    SDL_Color text_color = {200, 200, 200, 255};
    for (int y = 0; y < 2; y++) {
        if (handle->font) {
            SDL_Surface *surface = TTF_RenderText_Solid(handle->font, 
                                                       handle->lcd->display[y], 
                                                       text_color);
            if (surface) {
                SDL_Texture *texture = SDL_CreateTextureFromSurface(handle->renderer, surface);
                SDL_Rect text_rect = {30, 30 + y * 40, surface->w, surface->h};
                SDL_RenderCopy(handle->renderer, texture, NULL, &text_rect);
                SDL_FreeSurface(surface);
                SDL_DestroyTexture(texture);
            }
        }
    }
    
    s_sdl_render_hint(handle);

    SDL_RenderPresent(handle->renderer);
}

// Остальные функции остаются практически без изменений
void lcd1602_sdl_release(sdl_handle_t *handle) {
    if (!handle) return;
    
    if (handle->font) TTF_CloseFont(handle->font);
    if (handle->renderer) SDL_DestroyRenderer(handle->renderer);
    if (handle->window) SDL_DestroyWindow(handle->window);
    
    free(handle->lcd);
    free(handle);
    
    TTF_Quit();
    SDL_Quit();
}

void lcd1602_sdl_set_position_cb(sdl_handle_t *handle, void (*cb)(int)) {
    if (handle) handle->position_cb = cb;
}

void lcd1602_sdl_set_push_button_cb(sdl_handle_t *handle, void (*cb)(void)) {
    if (handle) handle->push_button_cb = cb;
}

void lcd1602_sdl_set_long_push_button_cb(sdl_handle_t *handle, void (*cb)(void)) {
    if (handle) handle->long_push_button_cb = cb;
}

void lcd1602_sdl_set_double_click_cb(sdl_handle_t *handle, void (*cb)(void)) {
    if (handle) handle->double_click_cb = cb;
}

void lcd1602_sdl_set_debounce(sdl_handle_t *handle, uint8_t debounce) {
    if (handle && handle->lcd) handle->lcd->debounce = debounce;
}

void lcd_sdl_clear(sdl_handle_t *handle) {
    if (!handle || !handle->lcd) return;
    
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            handle->lcd->display[y][x] = ' ';
        }
        handle->lcd->display[y][16] = '\0';
    }
    handle->lcd->cursor_x = 0;
    handle->lcd->cursor_y = 0;
}

void lcd_sdl_set_cursor(sdl_handle_t *handle, int x, int y) {
    if (handle && handle->lcd && x >= 0 && x < 16 && y >= 0 && y < 2) {
        handle->lcd->cursor_x = x;
        handle->lcd->cursor_y = y;
    }
}

void lcd_sdl_print_char(sdl_handle_t *handle, char ch) {
    if (handle && handle->lcd && handle->lcd->cursor_x < 16) {
        handle->lcd->display[handle->lcd->cursor_y][handle->lcd->cursor_x] = ch;
        handle->lcd->cursor_x++;
    }
}

void lcd_sdl_print_str(sdl_handle_t *handle, const char *str) {
    if (!handle || !str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        lcd_sdl_print_char(handle, str[i]);
    }
}