cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Keypad_Lock)

add_executable(Keypad_Lock main.c)

target_link_libraries(Keypad_Lock PJ_RPI)
