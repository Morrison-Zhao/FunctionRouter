cmake_minimum_required(VERSION 3.26)
project(MyEventBus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# 输出路径
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# 子模块
add_subdirectory(corelib)
add_subdirectory(extensions)

# 编译选项
option(ENABLE_EXAMPLES "Enable examples" ON)
if(ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

# 安装和导出
install(EXPORT MyEventBusTargets
    FILE MyEventBusTargets.cmake
    NAMESPACE MyEventBus::
    DESTINATION lib/cmake/MyEventBus
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/MyEventBusConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MyEventBusConfig.cmake"
    INSTALL_DESTINATION lib/cmake/MyEventBus
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MyEventBusConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MyEventBusConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MyEventBusConfigVersion.cmake"
    DESTINATION lib/cmake/MyEventBus
)
