cmake_minimum_required(VERSION 3.13)
project(convolution LANGUAGES CXX)

find_package(Celerity 0.4.1 REQUIRED)

add_executable(convolution convolution.cc)
set_property(TARGET convolution PROPERTY CXX_STANDARD 17)
add_celerity_to_target(TARGET convolution SOURCES convolution.cc)
