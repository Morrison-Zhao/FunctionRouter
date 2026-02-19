cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)

# ThreadPool 测试
add_executable(threadPool_test threadPool_test.cpp)
target_link_libraries(threadPool_test PRIVATE MyEventBusCore)
target_compile_features(threadPool_test PRIVATE cxx_std_17)

# EventDispatcher 测试
add_executable(EventDispatcher_test EventDispatcher_test.cpp)
target_link_libraries(EventDispatcher_test PRIVATE MyEventBusCore)
target_compile_features(EventDispatcher_test PRIVATE cxx_std_17)
