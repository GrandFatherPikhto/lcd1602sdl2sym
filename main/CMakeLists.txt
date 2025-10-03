cmake_minimum_required(VERSION 3.12)
project(lcd VERSION 1.0.0 LANGUAGES C)

# Настройка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Добавление исполняемого файла
add_executable(${PROJECT_NAME}
    src/main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LCD1602
)

# Подключение SDL2 к целевому файлу
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lcd1602/src/include
)

# Установка выходного каталога (опционально)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

