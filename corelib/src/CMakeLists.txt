# Define the interface library
add_library(FunctionRouterCore INTERFACE)
add_library(FunctionRouter::Core ALIAS FunctionRouterCore)

# Specify include directories
target_include_directories(FunctionRouterCore INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

# Install the library target
install(TARGETS FunctionRouterCore
    EXPORT FunctionRouterTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install header files
# corelib/include contains FunctionRouter directory
install(DIRECTORY ../include/
    DESTINATION include
)
