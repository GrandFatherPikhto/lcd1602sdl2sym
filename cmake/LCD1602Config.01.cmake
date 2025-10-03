# FindPackageLCD1602.cmake
#
# Находит библиотеку LCD1602 и устанавливает целевые объекты

# Поиск заголовочных файлов
find_path(LCD1602_INCLUDE_DIR
    NAMES lcd1602_sdl.h utils.h
    PATHS /home/yevst/Projects/CCPP/lcd1602sdl02sym/lcd1602/src/include
    NO_DEFAULT_PATH
)

# Поиск библиотеки
find_library(LCD1602_LIBRARY
    NAMES lcd1602
    PATHS /home/yevst/Projects/CCPP/lcd1602sdl02sym/build/lcd1602
    NO_DEFAULT_PATH
)

message(STATUS "Include Dir: ${LCD1602_INCLUDE_DIR}")
message(STATUS "Library Dir: ${LCD1602_LIBRARY}")


# Проверка найденных компонентов
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LCD1602
    REQUIRED_VARS LCD1602_LIBRARY LCD1602_INCLUDE_DIR
    # VERSION_VAR LCD1602_VERSION  # Опционально: если есть версия
)

# Проверка существования файлов
if(NOT EXISTS "${LCD1602_INCLUDE_DIR}/lcd1602_sdl.h")
    message(FATAL_ERROR "LCD1602 header not found: ${LCD1602_INCLUDE_DIR}/lcd1602_sdl.h")
endif()

if(NOT EXISTS "${LCD1602_LIBRARY}")
    message(FATAL_ERROR "LCD1602 library not found: ${LCD1602_LIBRARY}")
endif()

# Создание импортированной цели
if(LCD1602_FOUND AND NOT TARGET LCD1602::LCD1602)
    add_library(LCD1602::LCD1602 STATIC IMPORTED)
    set_target_properties(LCD1602::LCD1602 PROPERTIES
        IMPORTED_LOCATION "${LCD1602_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LCD1602_INCLUDE_DIR}"
    )
    
    # Добавляем зависимости SDL2
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    target_link_libraries(LCD1602::LCD1602 INTERFACE 
        SDL2::SDL2 
        SDL2_ttf::SDL2_ttf
    )    
endif()

# Установка переменных для обратной совместимости
if(LCD1602_FOUND)
    set(LCD1602_LIBRARIES ${LCD1602_LIBRARY})
    set(LCD1602_INCLUDE_DIRS ${LCD1602_INCLUDE_DIR})
endif()

mark_as_advanced(LCD1602_INCLUDE_DIR LCD1602_LIBRARY)
