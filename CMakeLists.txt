cmake_minimum_required(VERSION 3.10)
project(lab_3)

set(CMAKE_CXX_STANDARD 20)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

add_executable(generate_matrix  generate_matrix.cpp)
add_executable(lab_3 lab3_1.cpp)
add_executable(lab_3_2 lab3_2.cpp)
add_executable(lab_3_3 lab3_3.cpp)