cmake_minimum_required(VERSION 3.26)
set(ProjectName MyEventBus)

project(${ProjectName}
        VERSION 1.0.0
        LANGUAGES CXX
        DESCRIPTION "my event bus component")


# 全局配置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 禁用编译器扩展（如GNU的-std=gnu++17）


# 构建类型配置（Debug/Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# 添加主代码目录
add_subdirectory(coreLib)


option(ENABLE_EXAMPLES "Enable examples" ON)
option(ENABLE_TESTING "Enable testing" ON)



if(ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()


## 测试选项
#option(ENABLE_TESTING "Enable testing" ON)
#if(ENABLE_TESTING)
#    add_subdirectory(tests)   # 单元测试
#endif()