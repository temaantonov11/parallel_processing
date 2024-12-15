cmake_minimum_required(VERSION 3.10)
project(PARALELL_PROCESSING)

find_package(OpenCV REQUIRED)

add_executable(core_exec core.cpp)
target_link_libraries(core_exec ${OpenCV_LIBS})