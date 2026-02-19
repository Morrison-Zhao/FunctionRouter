cmake_minimum_required(VERSION 3.26)

# 定义一个接口库，方便其他模块链接
add_library(MyEventBusCore INTERFACE)

# 指定头文件搜索路径
target_include_directories(MyEventBusCore INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_TESTING)
    add_subdirectory(tests)
endif()
