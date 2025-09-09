#include "lcd1602_sdl.h"
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

#define LCD_EVENT_QUEUE_SIZE 0x80
#define MAX_FONT_PATH 1024

typedef struct lcd1602_handle {
    char display[2][17]; 
    int cursor_x; 
    int cursor_y; 
    bool backlight; 
    bool blink; 
    int contrast; 
    int pos;
} lcd1602_handle_t;

typedef struct {
    lcd_command_t cmd;
    int x, y;      // для set_cursor
    char ch;       // для print_char
    char buf[32];  // для print_str (ограничим строку 31 символ)
} lcd_event_t;

typedef enum {
    EVT_NONE = 0,
    EVT_KEY_DOWN,
    EVT_KEY_UP,
    EVT_MOUSE_BUTTON_DOWN,
    EVT_MOUSE_BUTTON_UP,
    EVT_MOUSE_MOTION,
    EVT_QUIT
} sdl_event_type_t;

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
    sdl_event_t events[LCD_EVENT_QUEUE_SIZE];
    int head;
    int tail;
    SDL_mutex *mutex;
    SDL_cond  *cond;
} sdl_event_queue_t;

typedef struct poll_event {
    SDL_Keycode last_key;
    Uint32 key_press_time;
    bool long_press_detected;
    bool double_click_detected;
    Uint32 blink;
    Uint32 last_blink;
    SDL_Event event;
} poll_event_t;

typedef struct sdl_handle {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Thread* sdl_thread;
    char fontPath[MAX_FONT_PATH];
    TTF_Font *font;
    lcd1602_handle_t lcd;

    bool running;
} sdl_handle_t;

static void s_lcd_init(sdl_handle_t *handle);
static bool s_sdl_lcd_init(sdl_handle_t *handle);
static bool s_sdl_font_init(sdl_handle_t *handle);
static bool s_sdl_poll_event_init (sdl_handle_t *handle);
static bool s_sdl_event_queue_init(sdl_handle_t *handle);
static bool s_dequeue_event(sdl_handle_t *handle, lcd_event_t *event);
static void s_enqueue_event(sdl_handle_t *handle, lcd_event_t *event);
static void s_sdl_lcd_render(sdl_handle_t *handle);
static void s_handle_event(sdl_handle_t *handle, lcd_event_t *event);
static void s_handle_key_press(sdl_handle_t *handle, poll_event_t *event);
static void s_lcd_sdl_poll_event(sdl_handle_t *handle, poll_event_t *event);
static void s_handle_key_press(sdl_handle_t *handle, poll_event_t *event);
static void s_handle_menu_navigation(sdl_handle_t *handle);
static void s_signal_handler(int sig);

sdl_handle_t *lcd1602_sdl_create(const char *title, int width, int height)
{
    sdl_handle_t *handle = (sdl_handle_t *) malloc(sizeof(sdl_handle_t));

    s_lcd_init(handle);

    if(s_sdl_lcd_init(handle) == false)
        return 0;

    if(s_sdl_poll_event_init(handle) == false)
        return 0;

    strncpy(handle->fontPath, "/usr/share/fonts/truetype/freefont/FreeMono.ttf", MAX_FONT_PATH);
    if (s_sdl_font_init(handle) == false)
        return 0;

    s_sdl_event_queue_init(handle);


    lcd_event_t event = {0};
    poll_event_t poll_event = {0};

/*    
    while (handle.running) {
        s_lcd_sdl_poll_event(&handle, &poll_event);
        s_handle_event(&handle, &event);
        s_sdl_lcd_render(&handle);
        SDL_Delay(20);
    }
*/

    // s_sdl_lcd_destroy(&handle);

    return handle;
}

static void s_lcd_init(sdl_handle_t *handle) {
    // handle->lcd = (lcd1602_handle_t *) malloc(sizeof(lcd1602_handle_t));

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            handle->lcd.display[y][x] = ' ';
        }
        handle->lcd.display[y][16] = '\0';
    }
    handle->lcd.cursor_x = 0;
    handle->lcd.cursor_y = 0;
    handle->lcd.backlight = true;
    handle->lcd.blink = false;
    handle->lcd.contrast = 50;
}

static bool s_sdl_lcd_init(sdl_handle_t *handle)
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

    handle->window = SDL_CreateWindow("LCD1602 Simulator with Menu", 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         360, 150, 
                                         SDL_WINDOW_SHOWN);
    
    if (!handle->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    handle->renderer = SDL_CreateRenderer(handle->window, -1, SDL_RENDERER_ACCELERATED);

    if (!handle->renderer) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    handle->running = true;

    return true;
}

static bool s_sdl_font_init(sdl_handle_t *handle)
{
    handle->font = TTF_OpenFont(handle->fontPath, 22);
    
    if (!handle->font) {
        // Попробуем найти стандартный шрифт
        handle->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20);
        if (!handle->font) {
            handle->font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 20);
            if (!handle->font) {
                printf("Using default SDL font\n");
                return false;
            }
        }
    }

    return true;
}

static bool s_sdl_event_queue_init(sdl_handle_t *handle) {
    // handle->queue = (lcd_event_queue_t *) malloc(sizeof(lcd_event_queue_t));
    // if (handle->queue == 0)
    //     return false;

    handle->queue.head = handle->queue.tail = 0;
    handle->queue.mutex = SDL_CreateMutex();
    if (handle->queue.mutex == 0)
        return false;

    handle->queue.cond  = SDL_CreateCond();
    if (handle->queue.cond == 0)
        return false;

    return true;
}

static bool s_sdl_poll_event_init (sdl_handle_t *handle) {
    // handle->poll_event = (poll_event_t *) malloc(sizeof(poll_event_t));
    // if (handle->poll_event == 0)
    //     return false;
    return true;    
}

#if 1
static bool s_dequeue_event(sdl_handle_t *handle, lcd_event_t *out) {
    bool got = false;

    SDL_LockMutex(handle->queue.mutex);

    if (handle->queue.head == handle->queue.tail) {
        // Ждём максимум 10 мс
        SDL_CondWaitTimeout(handle->queue.cond, handle->queue.mutex, 20);
    }

    if (handle->queue.head != handle->queue.tail) {
        *out = handle->queue.events[handle->queue.tail];
        handle->queue.tail = (handle->queue.tail + 1) % LCD_EVENT_QUEUE_SIZE;
        got = true;
    }

    SDL_UnlockMutex(handle->queue.mutex);
    return got;
}
#endif

#if 0
static bool s_dequeue_event(sdl_handle_t *handle, lcd_event_t *event) { 
    SDL_LockMutex(handle->queue.mutex); 
    if (handle->queue.head == handle->queue.tail) { 
        SDL_CondWait(handle->queue.cond, handle->queue.mutex); 
    } 
    
    *(event) = handle->queue.events[handle->queue.tail]; 
    handle->queue.tail = (handle->queue.tail + 1) % LCD_EVENT_QUEUE_SIZE; 
    SDL_UnlockMutex(handle->queue.mutex); 
    
    return true; 
}
#endif

// Поток-обработчик: читает SDL_PollEvent и кладет в очередь
static int SDLCALL event_thread(void *arg) {
    struct {
        SDL_Window *win;
        EventQueue *queue;
        bool *running;
    } *ctx = arg;

    SDL_Event e;
    while (*(ctx->running)) {
        // Пытаемся тормознуть цикл без busy-wait: ждем 16ms максимум
        while (SDL_PollEvent(&e)) {
            AppEvent ev;
            bool push = true;
            switch (e.type) {
                case SDL_KEYDOWN:
                    ev.type = EVT_KEY_DOWN;
                    ev.data.key.key = e.key.keysym.sym;
                    ev.data.key.scancode = e.key.keysym.scancode;
                    ev.data.key.mod = (uint8_t)e.key.keysym.mod;
                    break;
                case SDL_KEYUP:
                    ev.type = EVT_KEY_UP;
                    ev.data.key.key = e.key.keysym.sym;
                    ev.data.key.scancode = e.key.keysym.scancode;
                    ev.data.key.mod = (uint8_t)e.key.keysym.mod;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    ev.type = EVT_MOUSE_BUTTON_DOWN;
                    ev.data.mouse_btn.button = (uint8_t)e.button.button;
                    ev.data.mouse_btn.x = e.button.x;
                    ev.data.mouse_btn.y = e.button.y;
                    break;
                case SDL_MOUSEBUTTONUP:
                    ev.type = EVT_MOUSE_BUTTON_UP;
                    ev.data.mouse_btn.button = (uint8_t)e.button.button;
                    ev.data.mouse_btn.x = e.button.x;
                    ev.data.mouse_btn.y = e.button.y;
                    break;
                case SDL_MOUSEMOTION:
                    ev.type = EVT_MOUSE_MOTION;
                    ev.data.mouse_motion.x = e.motion.x;
                    ev.data.mouse_motion.y = e.motion.y;
                    ev.data.mouse_motion.xrel = e.motion.xrel;
                    ev.data.mouse_motion.yrel = e.motion.yrel;
                    ev.data.mouse_motion.buttons = e.motion.state;
                    break;
                case SDL_QUIT:
                    ev.type = EVT_QUIT;
                    push = true;
                    break;
                default:
                    push = false;
                    break;
            }
            if (push) {
                queue_push(ctx->queue, &ev);
            }
        }
        SDL_Delay(5); // небольшая пауза, чтобы не перегружать CPU
    }
    return 0;
}


static void s_enqueue_event(sdl_handle_t *handle, lcd_event_t *event) {
    SDL_LockMutex(handle->queue.mutex);

    int next = (handle->queue.head + 1) % LCD_EVENT_QUEUE_SIZE;
    if (next != handle->queue.tail) {
        handle->queue.events[handle->queue.head] = *event;
        handle->queue.head = next;
        SDL_CondSignal(handle->queue.cond);
    } else {
        // очередь переполнена → можно проигнорировать или расширить
        printf("LCD event queue overflow!\n");
    }

    SDL_UnlockMutex(handle->queue.mutex);
}

bool lcd1602_sdl_next_tick(sdl_handle_t *handle) {
    // printf("%s:%d running: %d\n", __FILE__, __LINE__, handle->running);
    s_lcd_sdl_poll_event(handle, &handle->poll_event);
    s_handle_event(handle, &handle->event);
    s_sdl_lcd_render(handle);

    return handle->running;
}

static void s_sdl_lcd_render(sdl_handle_t *handle) {
    // Фон
    SDL_SetRenderDrawColor(handle->renderer, 40, 40, 40, 255);
    SDL_RenderClear(handle->renderer);
    
    // Дисплей LCD
    SDL_Rect lcd_rect = {20, 20, 320, 80};
    SDL_SetRenderDrawColor(handle->renderer, 0, 80, 0, 255);
    SDL_RenderFillRect(handle->renderer, &lcd_rect);
    SDL_SetRenderDrawColor(handle->renderer, 0, 60, 0, 255);
    SDL_RenderDrawRect(handle->renderer, &lcd_rect);
    
    // Текст на дисплее
    SDL_Color text_color = {200, 200, 200, 255};
    for (int y = 0; y < 2; y++) {
        SDL_Surface *surface = TTF_RenderText_Solid(handle->font, handle->lcd.display[y], text_color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(handle->renderer, surface);
            SDL_Rect text_rect = {30, 30 + y * 40, surface->w, surface->h};
            SDL_RenderCopy(handle->renderer, texture, NULL, &text_rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }
    
    // Курсор
    if (handle->lcd.blink) {
        SDL_Rect cursor_rect = {30 + handle->lcd.cursor_x * 19, 30 + handle->lcd.cursor_y * 40, 2, 20};
        SDL_SetRenderDrawColor(handle->renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(handle->renderer, &cursor_rect);
    }
    
    // Подсказки управления
    SDL_Color hint_color = {150, 150, 150, 255};
    const char *hints = "UP/DOWN: Navigate  ENTER: Select  D: Long press";
    SDL_Surface *hint_surface = TTF_RenderText_Solid(handle->font, hints, hint_color);
    if (hint_surface) {
        SDL_Texture *hint_texture = SDL_CreateTextureFromSurface(handle->renderer, hint_surface);
        SDL_Rect hint_rect = {20, 110, hint_surface->w, hint_surface->h};
        SDL_RenderCopy(handle->renderer, hint_texture, NULL, &hint_rect);
        SDL_FreeSurface(hint_surface);
        SDL_DestroyTexture(hint_texture);
    }
    
    SDL_RenderPresent(handle->renderer);
}

static void s_lcd_sdl_poll_event(sdl_handle_t *handle, poll_event_t *event) {
    if (SDL_PollEvent(&(event->event))) {
        switch (event->event.type) {
            case SDL_QUIT:
                handle->running = false;
                break;
                
            case SDL_KEYDOWN:
                if (event->event.key.keysym.sym != SDLK_UNKNOWN) {
                    event->last_key = event->event.key.keysym.sym;
                    event->key_press_time = SDL_GetTicks();
                    event->long_press_detected = false;
                }
                break;
                
            case SDL_KEYUP:
                if (!event->long_press_detected) {
                    s_handle_key_press(handle, event);
                }
                event->last_key = SDLK_UNKNOWN;
                break;
        }

        // Проверка долгого нажатия
        if (event->last_key != SDLK_UNKNOWN && SDL_GetTicks() - event->key_press_time > 800) {
            s_handle_key_press(handle, event);
            event->long_press_detected = true;
            event->last_key = SDLK_UNKNOWN;
        }
        
        // Мигание курсора
        if (SDL_GetTicks() - event->last_blink > 500) {
            handle->lcd.blink = !handle->lcd.blink;
            event->last_blink = SDL_GetTicks();
        }
    }
}

static void s_handle_event(sdl_handle_t *handle, lcd_event_t *event)
{
    if (s_dequeue_event(handle, event))
    {
        switch (event->cmd) {
            case LCD_CMD_CLEAR:
                for (int y = 0; y < 2; y++) {
                    for (int x = 0; x < 16; x++)
                        handle->lcd.display[y][x] = ' ';
                    handle->lcd.display[y][16] = '\0';
                }
                handle->lcd.cursor_x = handle->lcd.cursor_y = 0;
                break;

            case LCD_CMD_SET_CURSOR:
                if (event->x >= 0 
                    && event->x < 16 
                    && event->y >= 0 
                    && event->y < 2) {
                        handle->lcd.cursor_x = event->x;
                        handle->lcd.cursor_y = event->y;
                }
                break;

            case LCD_CMD_PRINT_CHAR:
                if (handle->lcd.cursor_x < 16) {
                    handle->lcd.display[handle->lcd.cursor_y][handle->lcd.cursor_x++] = event->ch;
                }
                break;

            case LCD_CMD_PRINT_STR:
                for (int i = 0; event->buf[i] && handle->lcd.cursor_x < 16; i++) {
                    handle->lcd.display[handle->lcd.cursor_y][handle->lcd.cursor_x++] = event->buf[i];
                }
                break;
            }
    }
}

static void s_handle_key_press(sdl_handle_t *handle, poll_event_t *event) {
    switch (event->last_key) {
        case SDLK_UP:
            break;
            
        case SDLK_DOWN:
            break;
            
        case SDLK_RETURN:
            break;
            
        case SDLK_d:
            break;
        
        case SDLK_q:
            printf("%s:%d Quit\n", __FILE__, __LINE__);
            handle->running = false;
            break;
        
        case SDLK_ESCAPE:
            
            break;
    }
    
    s_handle_menu_navigation(handle);
}

static void s_handle_menu_navigation(sdl_handle_t *handle)
{

}

static void s_signal_handler(int sig) {
    void *array[10];
    size_t size;

    // Получаем backtrace
    size = backtrace(array, 10);
    
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

void lcd1602_sdl_release(sdl_handle_t *handle)
{
    /* Остановить/разблокировать очередь, если нужно, чтобы другие потоки не встали в блок */
    if (handle->queue.mutex && handle->queue.cond) {
        SDL_LockMutex(handle->queue.mutex);
        SDL_CondBroadcast(handle->queue.cond);
        SDL_UnlockMutex(handle->queue.mutex);
    }

    /* Уничтожаем графику/шрифты в правильном порядке */
    if (handle->renderer) {
        SDL_DestroyRenderer(handle->renderer);
        handle->renderer = NULL;
    }

    if (handle->window) {
        SDL_DestroyWindow(handle->window);
        handle->window = NULL;
    }

    if (handle->font) {
        TTF_CloseFont(handle->font);
        handle->font = NULL;
    }

    TTF_Quit();
    SDL_Quit();
#if 0    
#endif

    /* Очистка внутренних аллокаций */
#if 0    
    if (handle->lcd) {
        free(handle->lcd);
        handle->lcd = NULL;
    }

    if (handle->queue) {
        if (handle->queue.mutex) { SDL_DestroyMutex(handle->queue.mutex); handle->queue.mutex = NULL; }
        if (handle->queue.cond)  { SDL_DestroyCond(handle->queue.cond);   handle->queue.cond  = NULL; }
        free(handle->queue);
        handle->queue = NULL;
    }

    if (handle->poll_event) {
        free(handle->poll_event);
        handle->poll_event = NULL;
    }

    // /* Освобождаем сам handle ТОЛЬКО если он был создан в потоке */
    // if (handle->self_allocated) {
    //     free(handle);
    //     handle = NULL;
    // }
#endif    
}
