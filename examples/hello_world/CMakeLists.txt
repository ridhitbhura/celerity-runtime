cmake_minimum_required(VERSION 3.13)
project(hello_world LANGUAGES CXX)

find_package(Celerity 0.4.1 REQUIRED)

add_executable(hello_world hello_world.cc)
set_property(TARGET hello_world PROPERTY CXX_STANDARD 17)
add_celerity_to_target(TARGET hello_world SOURCES hello_world.cc)
