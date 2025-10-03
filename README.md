# LCD1602 Simulator

Виртуальный симулятор LCD-дисплея 16x2 на базе SDL2, предназначенный для разработки и тестирования ПО без реального оборудования.

## Особенности

- 🖥️ Визуализация LCD-дисплея 16x2 символов
- ⌨️ Управление через клавиатуру (эмуляция энкодера и кнопок)
- 🔄 Поддержка callback-функций для обработки событий
- 🎨 Реалистичное отображение с зеленой LCD-подсветкой
- 🚀 Кроссплатформенность (Windows/Linux/macOS)

## Сборка проекта

### Зависимости

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev cmake build-essential
```

**Windows (MinGW):**
```bash
# Установите SDL2 и SDL2_ttf через vcpkg или скачайте библиотеки вручную
```

**macOS:**
```bash
brew install sdl2 sdl2_ttf cmake
```

### Компиляция

```bash
# Клонирование репозитория (если необходимо)
git clone <repository-url>
cd lcd1602sdl02sym

# Создание build директории
mkdir build && cd build

# Конфигурация и сборка
cmake ..
make

# Запуск примера
./bin/menu
```

Или используйте цель `run`:
```bash
make run
```

## Использование в вашем проекте

### 1. Подключение библиотеки

Добавьте в ваш `CMakeLists.txt`:
```cmake
find_package(LCD1602 REQUIRED)
target_link_libraries(your_target LCD1602::LCD1602)
```

### 2. Базовый пример

```c
#include "lcd1602_sdl.h"
#include <unistd.h>

// Callback-функции
void on_position_change(int new_pos) {
    printf("Position: %d\n", new_pos);
}

void on_button_push() {
    printf("Button pressed\n");
}

int main() {
    // Создание симулятора
    sdl_handle_t *lcd = lcd1602_sdl_create("My LCD Display", 360, 150);
    if (!lcd) return -1;

    // Настройка callback-ов
    lcd1602_sdl_set_position_cb(lcd, on_position_change);
    lcd1602_sdl_set_push_button_cb(lcd, on_button_push);

    // Главный цикл
    while (lcd1602_sdl_next_tick(lcd)) {
        // Ваша логика здесь
        lcd_sdl_clear(lcd);
        lcd_sdl_set_cursor(lcd, 0, 0);
        lcd_sdl_print_str(lcd, "Hello World!");
        lcd_sdl_set_cursor(lcd, 0, 1);
        lcd_sdl_print_str(lcd, "Position: 123");
        
        usleep(10000); // 10ms
    }

    // Освобождение ресурсов
    lcd1602_sdl_release(lcd);
    return 0;
}
```

## Управление

| Клавиша | Действие |
|---------|----------|
| `↑`/`↓` | Эмуляция энкодера (изменение позиции) |
| `Enter` | Короткое нажатие кнопки |
| `L` | Длинное нажатие кнопки |
| `D` | Двойное нажатие кнопки |
| `Q` | Выход из программы |

## API Reference

### Основные функции

```c
// Создание и уничтожение
sdl_handle_t* lcd1602_sdl_create(const char* title, int width, int height);
void lcd1602_sdl_release(sdl_handle_t* handle);

// Главный цикл
bool lcd1602_sdl_next_tick(sdl_handle_t* handle);

// Управление дисплеем
void lcd_sdl_clear(sdl_handle_t* handle);
void lcd_sdl_set_cursor(sdl_handle_t* handle, int x, int y);
void lcd_sdl_print_char(sdl_handle_t* handle, char ch);
void lcd_sdl_print_str(sdl_handle_t* handle, const char* str);

// Настройка callback-ов
void lcd1602_sdl_set_position_cb(sdl_handle_t* handle, void (*cb)(int));
void lcd1602_sdl_set_push_button_cb(sdl_handle_t* handle, void (*cb)(void));
void lcd1602_sdl_set_long_push_button_cb(sdl_handle_t* handle, void (*cb)(void));
void lcd1602_sdl_set_double_click_cb(sdl_handle_t* handle, void (*cb)(void));

// Настройка параметров
void lcd1602_sdl_set_debounce(sdl_handle_t* handle, uint8_t debounce);
```

### Callback-функции

- **Position Callback**: Вызывается при изменении позиции энкодера
- **Push Button Callback**: Короткое нажатие кнопки
- **Long Push Callback**: Длинное нажатие (>1 сек)
- **Double Click Callback**: Двойное нажатие

## Структура проекта

```
lcd1602sdl02sym/
├── CMakeLists.txt
├── README.md
├── cmake
│   ├── LCD1602Config.cmake
│   └── lcd1602-config.cmake
├── lcd1602
│   ├── CMakeLists.txt
│   └── src
│       ├── include
│       │   ├── lcd1602_sdl.h
│       │   ├── lcd1602_sdl_types.h
│       │   └── utils.h
│       ├── lcd1602_sdl.c
│       └── utils.c
├── lcd1602sdl02sym.code-workspace
├── main
│   ├── CMakeLists.txt
│   └── src
│       ├── include
│       └── main.c
├── resources
│   ├── 5x8_lcd_hd44780u_a02
│   │   └── 5x8_LCD_HD44780U_A02
│   │       ├── 5x8_lcd_hd44780u_a02.ttf
│   │       ├── 5x8_lcd_hd44780u_a02.ttf:Zone.Identifier
│   │       ├── readme.txt
│   │       └── readme.txt:Zone.Identifier
│   └── lcd_font.ttf
```

## Отладка

Проект включает обработчик сигналов для отладки segmentation faults. При падении программы выводится backtrace для идентификации проблемы.

```bash
# Для расшифровки backtrace используйте:
addr2line -e ./menu <address>
```

## Лицензия

Это учебный проект, распространяемый под лицензией MIT. 
Вы можете свободно использовать, модифицировать и распространять код 
с указанием авторства.

## Вклад в проект

Приветствуются pull requests и issue reports для улучшения функциональности и исправления ошибок.
```
