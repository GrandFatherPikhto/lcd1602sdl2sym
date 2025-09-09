#include "lcd1602_sdl.h"
#include "lcd1602_sdl_types.h"

static sdl_handle_t *s_lcd1602_sdl_init(const char *title, int width, int height);
static bool s_sdl_init(sdl_handle_t *handle);
static lcd1602_handle_t *s_lcd1602_init(void);
static sdl_event_queue_t *s_sdl_queue_init(void);
static bool s_queue_push(sdl_event_queue_t *q, const sdl_event_t *ev);
static bool s_queue_pop(sdl_event_queue_t *q, sdl_event_t *out);
static void s_sdl_render(sdl_handle_t *handle);
static int SDLCALL s_event_thread(void *arg);

sdl_handle_t *lcd1602_sdl_create(const char *title, int width, int height)
{
    return s_lcd1602_sdl_init(title, width, height);
}

bool lcd1602_sdl_next_tick(sdl_handle_t *handle)
{
    // Обработка событий, поступивших из очереди
    sdl_event_t ev;
    // Попытаемся не блокировать основной цикл на чтение
    sdl_event_queue_t *queue = handle->queue;
    while (queue->count > 0) {
        if (s_queue_pop(queue, &ev)) {
            switch (ev.type) {
                case EVT_KEY_DOWN:
                    // обработка нажатия клавиши
                    // printf("KeyDown: %d\n", (int)ev.data.key.key);
                    break;
                case EVT_KEY_UP:
                    printf("KeyUp: %d\n", (int)ev.data.key.key);
                    if (ev.data.key.key == SDLK_q)
                    {
                        handle->running = false;
                    }
                    break;
                case EVT_MOUSE_BUTTON_DOWN:
                    // printf("MouseDown: btn %u at (%d,%d)\n",
                    //        ev.data.mouse_btn.button, ev.data.mouse_btn.x, ev.data.mouse_btn.y);
                    break;
                case EVT_MOUSE_BUTTON_UP:
                    // printf("MouseUp: btn %u at (%d,%d)\n",
                    //        ev.data.mouse_btn.button, ev.data.mouse_btn.x, ev.data.mouse_btn.y);
                    break;
                case EVT_MOUSE_MOTION:
                    // printf("MouseMotion: (%d,%d) rel (%d,%d)\n",
                    //        ev.data.mouse_motion.x, ev.data.mouse_motion.y,
                    //        ev.data.mouse_motion.xrel, ev.data.mouse_motion.yrel);
                    break;
                case EVT_QUIT:
                    handle->running = false;
                    printf("Handle Quit\n");
                    lcd1602_sdl_release(handle);
                    break;
                default:
                    break;
            }
        }
    }

    s_sdl_render(handle);

    return handle->running;
}

//< Внутренние статические функции

static lcd1602_handle_t *s_lcd1602_init(void)
{
    lcd1602_handle_t *handle = (lcd1602_handle_t *) malloc(sizeof(lcd1602_handle_t));
    if (handle == 0)
    {
        free(handle);
        return 0;
    }

    memset(handle, 0, sizeof(lcd1602_handle_t));
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16; x++) {
            handle->display[y][x] = ' ';
        }
        handle->display[y][16] = '\0';
    }
    handle->cursor_x = 0;
    handle->cursor_y = 0;
    handle->backlight = true;
    handle->blink = false;
    handle->contrast = 50;

    return handle;
}

static sdl_event_queue_t *s_sdl_queue_init(void) {
    sdl_event_queue_t *queue = (sdl_event_queue_t *) malloc(sizeof(sdl_event_queue_t));

    if (queue == 0) {
        return 0;
    }

    memset(queue, 0, sizeof(sdl_event_queue_t));

    queue->mutex = SDL_CreateMutex();
    if (queue->mutex == 0) {
        free(queue);
        queue = 0;
        return 0;
    }

    queue->cond_not_empty = SDL_CreateCond();
    
    if (queue->cond_not_empty == 0) {
        SDL_DestroyMutex(queue->mutex);
        free(queue);
        queue = 0;
        return 0;
    }

    queue->cond_not_full = SDL_CreateCond();
    
    if (queue->cond_not_full == 0) {
        SDL_DestroyMutex(queue->mutex);
        SDL_DestroyCond(queue->cond_not_empty);
        free(queue);
        queue = 0;
        return 0;
    }

    return queue;
}

static thread_context_t *s_thread_ctx_init(sdl_handle_t *handle)
{
    thread_context_t *context = (thread_context_t *) malloc(sizeof(thread_context_t));
    if (context == 0)
        return 0;
    
    context->win = handle->window;
    context->queue = handle->queue;
    context->running = &(handle->running);

    return context;
}

static sdl_handle_t *s_lcd1602_sdl_init(const char *title, int width, int height)
{
    sdl_handle_t *handle = (sdl_handle_t *) malloc(sizeof(sdl_handle_t));
    if (handle == 0)
        return 0;
    memset(handle, 0, sizeof(sdl_handle_t));

    handle->lcd = s_lcd1602_init();
    if (handle->lcd == 0) {
        free(handle);
        handle = 0;
        return 0;
    }

    handle->queue = s_sdl_queue_init();
    if(handle->queue == 0) {
        free(handle->lcd);
        free(handle);
        handle = 0;
        return 0;
    }

    handle->running = true;

    handle->thread_ctx = s_thread_ctx_init(handle);
    if(handle->thread_ctx == 0) {
        free(handle->lcd);
        free(handle->queue);
        free(handle);
        handle = 0;
        return 0;
    }

    handle->thread = SDL_CreateThread(s_event_thread, "event_thread", handle->thread_ctx);

    if (!s_sdl_init(handle)) {
        lcd1602_sdl_release(handle);
        return 0;
    }

    return handle;
}

static bool s_sdl_init(sdl_handle_t *handle) {
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

static void s_sdl_render(sdl_handle_t *handle) {
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
        SDL_Surface *surface = TTF_RenderText_Solid(handle->font, handle->lcd->display[y], text_color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(handle->renderer, surface);
            SDL_Rect text_rect = {30, 30 + y * 40, surface->w, surface->h};
            SDL_RenderCopy(handle->renderer, texture, NULL, &text_rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }
    
    // Курсор
    if (handle->lcd->blink) {
        SDL_Rect cursor_rect = {30 + handle->lcd->cursor_x * 19, 30 + handle->lcd->cursor_y * 40, 2, 20};
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


static bool s_queue_push(sdl_event_queue_t *q, const sdl_event_t *ev) {
    bool ok = true;
    SDL_LockMutex(q->mutex);
    while (q->count >= LCD_EVENT_QUEUE_SIZE) {
        // wait until space is available
        SDL_CondWait(q->cond_not_full, q->mutex);
    }
    q->buffer[q->tail] = *ev;
    q->tail = (q->tail + 1) % LCD_EVENT_QUEUE_SIZE;
    q->count++;
    SDL_UnlockMutex(q->mutex);
    SDL_CondBroadcast(q->cond_not_empty);

    return ok;
}

static bool s_queue_pop(sdl_event_queue_t *q, sdl_event_t *out) {
    bool has = false;
    SDL_LockMutex(q->mutex);
    while (q->count == 0) {
        // No events, wait
        SDL_CondWait(q->cond_not_empty, q->mutex);
    }
    *out = q->buffer[q->head];
    q->head = (q->head + 1) % LCD_EVENT_QUEUE_SIZE;
    q->count--;
    has = true;
    SDL_UnlockMutex(q->mutex);
    SDL_CondBroadcast(q->cond_not_full);

    return has;
}


// Поток-обработчик: читает SDL_PollEvent и кладет в очередь
static int SDLCALL s_event_thread(void *arg) {
    thread_context_t *ctx = arg;
    // printf("%s:%d Thread Started. Runing = %d\n", __FILE__, __LINE__, *(ctx->running));
    SDL_Event e;
    while (*(ctx->running)) {
        // Пытаемся тормознуть цикл без busy-wait: ждем 16ms максимум
        while (SDL_PollEvent(&e)) {
            sdl_event_t ev;
            bool push = true;
            switch (e.type) {
                case SDL_KEYDOWN:
                    ev.type = EVT_KEY_DOWN;
                    ev.data.key.key = e.key.keysym.sym;
                    ev.data.key.scancode = e.key.keysym.scancode;
                    ev.data.key.mod = (uint8_t)e.key.keysym.mod;
                    printf("%s:%d %c\n", __FILE__, __LINE__,e.key.keysym.sym);
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
                // printf("%s:%d %p\n", __FILE__, __LINE__, ctx->queue);
                s_queue_push(ctx->queue, &ev);
            }
        }
        SDL_Delay(5); // небольшая пауза, чтобы не перегружать CPU
    }
    return 0;
}

void lcd1602_sdl_release(sdl_handle_t *handle)
{
    /* Остановить/разблокировать очередь, если нужно, чтобы другие потоки не встали в блок */
    if (handle->queue->mutex && handle->queue->cond_not_empty && handle->queue->cond_not_full) {
        SDL_LockMutex(handle->queue->mutex);
        SDL_CondBroadcast(handle->queue->cond_not_empty);
        SDL_CondBroadcast(handle->queue->cond_not_full);
        SDL_UnlockMutex(handle->queue->mutex);
    }

    /* Уничтожаем графику/шрифты в правильном порядке */
    if (handle->renderer != 0) {
        SDL_DestroyRenderer(handle->renderer);
        handle->renderer = NULL;
    }

    if (handle->window != 0) {
        SDL_DestroyWindow(handle->window);
        handle->window = NULL;
    }

    if (handle->font != 0) {
        TTF_CloseFont(handle->font);
        handle->font = NULL;
    }

    TTF_Quit();
    SDL_Quit();

    if (handle->queue != 0) {
        if (handle->queue->mutex) { 
            SDL_DestroyMutex(handle->queue->mutex); handle->queue->mutex = 0; 
        }

        if (handle->queue->cond_not_empty)  { 
            SDL_DestroyCond(handle->queue->cond_not_empty);   handle->queue->cond_not_empty  = 0; 
        }

        if (handle->queue->cond_not_full)  { 
            SDL_DestroyCond(handle->queue->cond_not_full);   handle->queue->cond_not_full  = 0; 
        }

        free(handle->queue);
        handle->queue = 0;
    }

    if (handle->lcd != 0)
    {
        free(handle->lcd);
        handle->lcd = 0;
    }

    if (handle->thread_ctx != 0)
    {
        free(handle->thread_ctx);
        handle->thread_ctx = 0;
    }

    if (handle)
    {
        free(handle);
        handle = 0;
    }
}

