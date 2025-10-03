#include "lcd1602_sdl.h"
#include "utils.h"
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <string.h>

#define LCD_STRING_LEN 0x10

char s_title_buf[LCD_STRING_LEN] = { 0 };
char s_value_buf[LCD_STRING_LEN] = { 0 };

// Callback-функции
void on_position_change(int new_pos) {
    printf("Position changed: %d\n", new_pos);
    // Здесь обновляем меню или что-то еще
    memset(s_title_buf, 0, LCD_STRING_LEN);
    memset(s_value_buf, 0, LCD_STRING_LEN);
    snprintf(s_title_buf, LCD_STRING_LEN, "Position");
    snprintf(s_value_buf, LCD_STRING_LEN, "Value: %d", new_pos);
}

void on_push_button() {
    printf("Button pushed\n");
    // Обработка короткого нажатия
    memset(s_title_buf, 0, LCD_STRING_LEN);
    memset(s_value_buf, 0, LCD_STRING_LEN);
    snprintf(s_title_buf, LCD_STRING_LEN, "Button");
    snprintf(s_value_buf, LCD_STRING_LEN, "Click");
}

void on_long_push_button() {
    printf("Long button push\n");
    // Обработка длинного нажатия
    memset(s_title_buf, 0, LCD_STRING_LEN);
    memset(s_value_buf, 0, LCD_STRING_LEN);
    snprintf(s_title_buf, LCD_STRING_LEN, "Button");
    snprintf(s_value_buf, LCD_STRING_LEN, "Long Click");
}

void on_double_click() {
    printf("Double click\n");
    // Обработка двойного нажатия
    memset(s_title_buf, 0, LCD_STRING_LEN);
    memset(s_value_buf, 0, LCD_STRING_LEN);
    snprintf(s_title_buf, LCD_STRING_LEN, "Button");
    snprintf(s_value_buf, LCD_STRING_LEN, "Double Click");
}

bool menu_is_dirty(void) {
    return true;
}

void menu_draw(sdl_handle_t *lcd) {
    lcd_sdl_clear(lcd);
    lcd_sdl_set_cursor(lcd, 0, 0);
    lcd_sdl_print_str(lcd, s_title_buf);
    lcd_sdl_set_cursor(lcd, 0, 1);
    lcd_sdl_print_str(lcd, s_value_buf);
}

int main() {
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);

    // Создаем экземпляр симулятора
    sdl_handle_t *lcd = lcd1602_sdl_create("LCD1602 Simulator", 360, 150);
    if (!lcd) {
        fprintf(stderr, "Failed to create LCD simulator\n");
        return 1;
    }

    // Устанавливаем callback-и
    lcd1602_sdl_set_position_cb(lcd, on_position_change);
    lcd1602_sdl_set_push_button_cb(lcd, on_push_button);
    lcd1602_sdl_set_long_push_button_cb(lcd, on_long_push_button);
    lcd1602_sdl_set_double_click_cb(lcd, on_double_click);

    // Устанавливаем дебаунс (опционально)
    lcd1602_sdl_set_debounce(lcd, 2);

    // Главный цикл
    while (lcd1602_sdl_next_tick(lcd)) {
        // Здесь может быть ваша логика, например, обновление меню
        // Но важно не блокировать надолго, чтобы симулятор оставался отзывчивым

        // Например, проверяем, нужно ли перерисовать меню
        if (menu_is_dirty()) {
            menu_draw(lcd);
        }

        // Небольшая пауза, чтобы не грузить CPU
        usleep(10000); // 10 мс
    }

    // Освобождаем ресурсы
    lcd1602_sdl_release(lcd);

    return 0;
}