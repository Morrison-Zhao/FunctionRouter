# Define the extensions library
add_library(FunctionRouterExtensions
    AsyncLogger.cpp
)

# Alias for internal usage
add_library(FunctionRouter::Extensions ALIAS FunctionRouterExtensions)

# Include directories
target_include_directories(FunctionRouterExtensions 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# C++ Standard
target_compile_features(FunctionRouterExtensions PUBLIC cxx_std_17)

# Install rules
install(TARGETS FunctionRouterExtensions
    EXPORT FunctionRouterTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY ../include/
    DESTINATION include
)
