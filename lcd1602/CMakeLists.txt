cmake_minimum_required(VERSION 3.12)
project(LCD1602 VERSION 1.0.0 LANGUAGES C)

# Настройка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Поиск библиотеки SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Добавление исполняемого файла
add_executable(lcd1602
    src/lcd1602_sdl.c
    src/main.c
)

# Подключение SDL2 к целевому файлу
target_include_directories(lcd1602 PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(lcd1602 
    # ${SDL2_LIBRARIES}
    SDL2::SDL2 
    SDL2_ttf::SDL2_ttf
    )

# Для Windows дополнительные настройки
if(WIN32)
    target_link_libraries(lcd1602 mingw32)
endif()

# Установка выходного каталога (опционально)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Добавление цели для запуска
add_custom_target(run
    COMMAND lcd1602
    DEPENDS lcd1602
    # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)