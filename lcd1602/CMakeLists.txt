cmake_minimum_required(VERSION 3.12)
project(LCD1602 VERSION 1.0.0 LANGUAGES C)

# Настройка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Поиск библиотеки SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_library(lcd1602
    src/lcd1602_sdl.c
    src/utils.c
)

# Добавление исполняемого файла
add_executable(menu
    src/lcd1602_sdl.c
    src/utils.c
    src/main.c
)

message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")

# Подключение SDL2 к целевому файлу
target_include_directories(menu PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_include_directories(lcd1602 PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

target_link_libraries(menu
    SDL2::SDL2 
    SDL2_ttf::SDL2_ttf
    )

target_link_libraries(lcd1602
    SDL2::SDL2 
    SDL2_ttf::SDL2_ttf
    )

# Для Windows дополнительные настройки
if(WIN32)
    target_link_libraries(menu mingw32)
    target_link_libraries(lcd1602 mingw32)
endif()

# Установка выходного каталога (опционально)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_custom_target(run
    COMMAND menu
    DEPENDS menu
    # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
