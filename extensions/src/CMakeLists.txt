# Define the extensions library
add_library(MyEventBusExtensions
    AsyncLogger.cpp
)

# Alias for internal usage
add_library(MyEventBus::Extensions ALIAS MyEventBusExtensions)

# Include directories
target_include_directories(MyEventBusExtensions 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# C++ Standard
target_compile_features(MyEventBusExtensions PUBLIC cxx_std_17)

# Install rules
install(TARGETS MyEventBusExtensions
    EXPORT MyEventBusTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY ../include/
    DESTINATION include
)
