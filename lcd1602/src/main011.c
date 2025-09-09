#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Состояния меню
typedef enum {
    MENU_MAIN,
    MENU_SETTINGS,
    MENU_BACKLIGHT,
    MENU_CONTRAST
} MenuState;

typedef struct {
    char display[2][17];
    int cursor_x;
    int cursor_y;
    bool backlight;
    bool blink;
    int contrast;
} LCD1602;

// Глобальные переменные для меню
MenuState current_menu = MENU_MAIN;
int menu_position = 0;
const char *menu_items[] = {"Settings", "Info", "Exit"};
const char *settings_items[] = {"Backlight", "Contrast", "Reset", "Back"};
int settings_value = 50; // Пример значения для настроек

bool lcd1602_init(void);
static void s_lcd_init(LCD1602 *lcd);
static void s_lcd_clear(LCD1602 *lcd);
static void s_lcd_set_cursor(LCD1602 *lcd, int x, int y);
static void s_lcd_print_char(LCD1602 *lcd, char ch);
static void s_lcd_print_str(LCD1602 *lcd, const char *str);
static void update_main_menu(LCD1602 *lcd);
static void update_settings_menu(LCD1602 *lcd);
static void update_backlight_menu(LCD1602 *lcd);
static void update_contrast_menu(LCD1602 *lcd);
static void handle_menu_navigation(LCD1602 *lcd);
static void handle_key_press(SDL_Keycode key, bool long_press, LCD1602 *lcd);
static void lcd_render(SDL_Renderer *renderer, TTF_Font *font, LCD1602 *lcd);

bool lcd1602_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }
    
    SDL_Window *window = SDL_CreateWindow("LCD1602 Simulator with Menu", 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         360, 150, 
                                         SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("font.ttf", 20);
    
    if (!font) {
        // Попробуем найти стандартный шрифт
        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20);
        if (!font) {
            font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 20);
            if (!font) {
                printf("Using default SDL font\n");
            }
        }
    }
    
    LCD1602 lcd;
    s_lcd_init(&lcd);
    update_main_menu(&lcd);
    
    bool running = true;
    SDL_Event event;
    Uint32 last_blink = SDL_GetTicks();
    Uint32 key_press_time = 0;
    SDL_Keycode last_key = SDLK_UNKNOWN;
    bool long_press_detected = false;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym != SDLK_UNKNOWN) {
                        last_key = event.key.keysym.sym;
                        key_press_time = SDL_GetTicks();
                        long_press_detected = false;
                    }
                    break;
                    
                case SDL_KEYUP:
                    if (!long_press_detected) {
                        handle_key_press(last_key, false, &lcd);
                    }
                    last_key = SDLK_UNKNOWN;
                    break;
            }
        }
        
        // Проверка долгого нажатия
        if (last_key != SDLK_UNKNOWN && SDL_GetTicks() - key_press_time > 800) {
            handle_key_press(last_key, true, &lcd);
            long_press_detected = true;
            last_key = SDLK_UNKNOWN;
        }
        
        // Мигание курсора
        if (SDL_GetTicks() - last_blink > 500) {
            lcd.blink = !lcd.blink;
            last_blink = SDL_GetTicks();
        }
        
        lcd_render(renderer, font, &lcd);
        SDL_Delay(16);
    }
    
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

static void s_lcd_init(LCD1602 *lcd) {
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
    lcd->contrast = 50;
}

static void s_lcd_clear(LCD1602 *lcd) {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            lcd->display[y][x] = ' ';
        }
        lcd->display[y][16] = '\0';
    }
    lcd->cursor_x = 0;
    lcd->cursor_y = 0;
}

static void s_lcd_set_cursor(LCD1602 *lcd, int x, int y) {
    if (x >= 0 && x < 16 && y >= 0 && y < 2) {
        lcd->cursor_x = x;
        lcd->cursor_y = y;
    }
}

static void s_lcd_print_char(LCD1602 *lcd, char ch) {
    if (lcd->cursor_x < 16) {
        lcd->display[lcd->cursor_y][lcd->cursor_x] = ch;
        lcd->cursor_x++;
    }
}

static void s_lcd_print_str(LCD1602 *lcd, const char *str) {
    for (int i = 0; str[i] != '\0' && lcd->cursor_x < 16; i++) {
        s_lcd_print_char(lcd, str[i]);
    }
}

// Функции для работы с меню
static void update_main_menu(LCD1602 *lcd) {
    s_lcd_clear(lcd);
    s_lcd_print_str(lcd, "MAIN MENU");
    s_lcd_set_cursor(lcd, 0, 1);
    
    if (menu_position < 3) {
        char line[17];
        snprintf(line, sizeof(line), ">%s", menu_items[menu_position]);
        s_lcd_print_str(lcd, line);
    }
}

static void update_settings_menu(LCD1602 *lcd) {
    s_lcd_clear(lcd);
    s_lcd_print_str(lcd, "SETTINGS");
    s_lcd_set_cursor(lcd, 0, 1);
    
    if (menu_position < 4) {
        char line[17];
        snprintf(line, sizeof(line), ">%s", settings_items[menu_position]);
        s_lcd_print_str(lcd, line);
    }
}


static void update_backlight_menu(LCD1602 *lcd) {
    s_lcd_clear(lcd);
    s_lcd_print_str(lcd, "BACKLIGHT");
    s_lcd_set_cursor(lcd, 0, 1);
    
    char line[17];
    if (lcd->backlight) {
        snprintf(line, sizeof(line), "ON  [======  ]");
    } else {
        snprintf(line, sizeof(line), "OFF [        ]");
    }
    s_lcd_print_str(lcd, line);
}

static void update_contrast_menu(LCD1602 *lcd) {
    s_lcd_clear(lcd);
    s_lcd_print_str(lcd, "CONTRAST");
    s_lcd_set_cursor(lcd, 0, 1);
    
    char line[17];
    int bars = lcd->contrast / 10;
    snprintf(line, sizeof(line), "%3d%% [", lcd->contrast);
    for (int i = 0; i < 10; i++) {
        strcat(line, i < bars ? "=" : " ");
    }
    strcat(line, "]");
    s_lcd_print_str(lcd, line);
}

static void handle_menu_navigation(LCD1602 *lcd) {
    switch (current_menu) {
        case MENU_MAIN:
            update_main_menu(lcd);
            break;
        case MENU_SETTINGS:
            update_settings_menu(lcd);
            break;
        case MENU_BACKLIGHT:
            update_backlight_menu(lcd);
            break;
        case MENU_CONTRAST:
            update_contrast_menu(lcd);
            break;
    }
}

static void handle_key_press(SDL_Keycode key, bool long_press, LCD1602 *lcd) {
    switch (key) {
        case SDLK_UP:
            if (current_menu == MENU_MAIN) {
                menu_position = (menu_position - 1 + 3) % 3;
            } else if (current_menu == MENU_SETTINGS) {
                menu_position = (menu_position - 1 + 4) % 4;
            } else if (current_menu == MENU_CONTRAST && long_press) {
                lcd->contrast = (lcd->contrast + 5) % 105;
                if (lcd->contrast > 100) lcd->contrast = 100;
            }
            break;
            
        case SDLK_DOWN:
            if (current_menu == MENU_MAIN) {
                menu_position = (menu_position + 1) % 3;
            } else if (current_menu == MENU_SETTINGS) {
                menu_position = (menu_position + 1) % 4;
            } else if (current_menu == MENU_CONTRAST && long_press) {
                lcd->contrast = (lcd->contrast - 5 + 105) % 105;
                if (lcd->contrast < 0) lcd->contrast = 0;
            }
            break;
            
        case SDLK_RETURN:
            if (current_menu == MENU_MAIN) {
                if (menu_position == 0) {
                    current_menu = MENU_SETTINGS;
                    menu_position = 0;
                } else if (menu_position == 1) {
                    s_lcd_clear(lcd);
                    s_lcd_print_str(lcd, "LCD1602 Simulator");
                    s_lcd_set_cursor(lcd, 0, 1);
                    s_lcd_print_str(lcd, "Ver 1.0");
                    return;
                } else if (menu_position == 2) {
                    // Exit
                    // В реальном приложении здесь был бы выход
                    s_lcd_clear(lcd);
                    s_lcd_print_str(lcd, "Goodbye!");
                    return;
                }
            } else if (current_menu == MENU_SETTINGS) {
                if (menu_position == 0) {
                    current_menu = MENU_BACKLIGHT;
                } else if (menu_position == 1) {
                    current_menu = MENU_CONTRAST;
                } else if (menu_position == 2) {
                    // Reset settings
                    lcd->backlight = true;
                    lcd->contrast = 50;
                    s_lcd_clear(lcd);
                    s_lcd_print_str(lcd, "Settings reset");
                    return;
                } else if (menu_position == 3) {
                    current_menu = MENU_MAIN;
                    menu_position = 0;
                }
            } else if (current_menu == MENU_BACKLIGHT) {
                lcd->backlight = !lcd->backlight;
            } else if (current_menu == MENU_CONTRAST) {
                // Короткое нажатие в меню контраста - возврат
                current_menu = MENU_SETTINGS;
                menu_position = 1;
            }
            break;
            
        case SDLK_d:
            if (long_press && current_menu == MENU_BACKLIGHT) {
                // Долгое нажатие D в меню подсветки
                lcd->backlight = !lcd->backlight;
            }
            break;
            
        case SDLK_ESCAPE:
            if (current_menu == MENU_SETTINGS || current_menu == MENU_BACKLIGHT || current_menu == MENU_CONTRAST) {
                current_menu = MENU_MAIN;
                menu_position = 0;
            }
            break;
    }
    
    handle_menu_navigation(lcd);
}

static void lcd_render(SDL_Renderer *renderer, TTF_Font *font, LCD1602 *lcd) {
    // Фон
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);
    
    // Дисплей LCD
    SDL_Rect lcd_rect = {20, 20, 320, 80};
    SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
    SDL_RenderFillRect(renderer, &lcd_rect);
    SDL_SetRenderDrawColor(renderer, 0, 60, 0, 255);
    SDL_RenderDrawRect(renderer, &lcd_rect);
    
    // Текст на дисплее
    SDL_Color text_color = {200, 200, 200, 255};
    for (int y = 0; y < 2; y++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, lcd->display[y], text_color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect text_rect = {30, 30 + y * 40, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &text_rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }
    
    // Курсор
    if (lcd->blink) {
        SDL_Rect cursor_rect = {30 + lcd->cursor_x * 19, 30 + lcd->cursor_y * 40, 2, 20};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &cursor_rect);
    }
    
    // Подсказки управления
    SDL_Color hint_color = {150, 150, 150, 255};
    const char *hints = "UP/DOWN: Navigate  ENTER: Select  D: Long press";
    SDL_Surface *hint_surface = TTF_RenderText_Solid(font, hints, hint_color);
    if (hint_surface) {
        SDL_Texture *hint_texture = SDL_CreateTextureFromSurface(renderer, hint_surface);
        SDL_Rect hint_rect = {20, 110, hint_surface->w, hint_surface->h};
        SDL_RenderCopy(renderer, hint_texture, NULL, &hint_rect);
        SDL_FreeSurface(hint_surface);
        SDL_DestroyTexture(hint_texture);
    }
    
    SDL_RenderPresent(renderer);
}

int main() {
    lcd1602_init();

    return 0;
}