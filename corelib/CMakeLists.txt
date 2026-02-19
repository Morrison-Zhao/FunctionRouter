add_subdirectory(src)

option(ENABLE_TESTING "Enable testing" ON)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
