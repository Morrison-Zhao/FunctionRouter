cmake_minimum_required(VERSION 3.26)

project(FunctionRouterExamples)

add_executable(simple_demo simple_demo.cpp)

# Link against the core library
target_link_libraries(simple_demo PRIVATE FunctionRouterCore)

# Ensure C++17
target_compile_features(simple_demo PRIVATE cxx_std_17)
