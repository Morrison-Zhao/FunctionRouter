# Define the interface library
add_library(MyEventBusCore INTERFACE)
add_library(MyEventBus::Core ALIAS MyEventBusCore)

# Specify include directories
target_include_directories(MyEventBusCore INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

# Install the library target
install(TARGETS MyEventBusCore
    EXPORT MyEventBusTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install header files
# corelib/include contains MyEventBus directory
install(DIRECTORY ../include/
    DESTINATION include
)
