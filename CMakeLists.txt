cmake_minimum_required(VERSION 3.10)
project(lab_2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR})

# Добавляем исполняемые файлы
add_executable(lab2_1 lab2_1.cpp)
add_executable(lab2_2 lab2_2.cpp)
add_executable(lab2_3 lab2_3.cpp)
