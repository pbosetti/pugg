cmake_minimum_required(VERSION 3.14...3.27)

add_library(pugg INTERFACE)
target_include_directories(pugg INTERFACE include)
target_compile_features(pugg INTERFACE cxx_std_17)
