cmake_minimum_required(VERSION 3.10)
project(LCD1602_Simulator)

# Тестирование надо включать на самом верхнем уровне, иначе работать не будет
include(CTest)
enable_testing()

# Добавляем подпроекты
add_subdirectory(main)
add_subdirectory(lcd1602)
# add_subdirectory(tests)

