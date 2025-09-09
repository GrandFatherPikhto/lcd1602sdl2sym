#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char display[2][17];  // +1 для нулевого терминатора
    int cursor_x;
    int cursor_y;
    bool backlight;
    bool blink;
} LCD1602;

void s_lcd_init(LCD1602 *lcd) {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            lcd->display[y][x] = ' ';
        }
        lcd->display[y][16] = '\0';
    }
    lcd->cursor_x = 0;
    lcd->cursor_y = 0;
    lcd->backlight = true;
    lcd->blink = false;
}

void s_lcd_clear(LCD1602 *lcd) {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            lcd->display[y][x] = ' ';
        }
        lcd->display[y][16] = '\0';
    }
    lcd->cursor_x = 0;
    lcd->cursor_y = 0;
}

void s_lcd_set_cursor(LCD1602 *lcd, int x, int y) {
    if (x >= 0 && x < 16 && y >= 0 && y < 2) {
        lcd->cursor_x = x;
        lcd->cursor_y = y;
    }
}

void s_lcd_print_char(LCD1602 *lcd, char ch) {
    if (lcd->cursor_x < 16) {
        lcd->display[lcd->cursor_y][lcd->cursor_x] = ch;
        lcd->cursor_x++;
    }
}

void s_lcd_print_str(LCD1602 *lcd, const char *str) {
    for (int i = 0; str[i] != '\0' && lcd->cursor_x < 16; i++) {
        s_lcd_print_char(lcd, str[i]);
    }
}

void lcd_render(SDL_Renderer *renderer, TTF_Font *font, LCD1602 *lcd) {
    // Очистка экрана
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);
    
    // Рисуем фон дисплея
    SDL_Rect lcd_rect = {20, 20, 320, 80};
    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
    SDL_RenderFillRect(renderer, &lcd_rect);
    
    // Отображаем текст
    SDL_Color text_color = {200, 200, 200, 255};
    for (int y = 0; y < 2; y++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, lcd->display[y], text_color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        SDL_Rect text_rect = {30, 30 + y * 40, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &text_rect);
        
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    
    // Рисуем курсор
    if (lcd->blink) {
        SDL_Rect cursor_rect = {30 + lcd->cursor_x * 19, 30 + lcd->cursor_y * 40, 2, 20};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &cursor_rect);
    }
    
    SDL_RenderPresent(renderer);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow("LCD1602 Simulator", 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         360, 120, 
                                         SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("font.ttf", 24);
    
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 24);
        if (!font) {
            printf("Using default SDL font\n");
        }
    }
    
    LCD1602 lcd;
    s_lcd_init(&lcd);
    
    // Тестовый вывод
    s_lcd_set_cursor(&lcd, 0, 0);
    s_lcd_print_str(&lcd, "Hello, World!");
    s_lcd_set_cursor(&lcd, 0, 1);
    s_lcd_print_str(&lcd, "LCD1602 Simulator");
    
    bool running = true;
    SDL_Event event;
    Uint32 last_blink = SDL_GetTicks();
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        
        // Мигание курсора
        if (SDL_GetTicks() - last_blink > 500) {
            lcd.blink = !lcd.blink;
            last_blink = SDL_GetTicks();
        }
        
        lcd_render(renderer, font, &lcd);
        SDL_Delay(16); // ~60 FPS
    }
    
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}