# List sources

set(SOURCES "test_conf.h" "main.cpp" "systems/mining.cpp")

# Find dependencies

find_package(GTest CONFIG REQUIRED)

# Link

include(GoogleTest)

add_executable(economy_tests ${SOURCES})
target_link_libraries(economy_tests PRIVATE GTest::gtest_main)
target_link_libraries(economy_tests PRIVATE Economy::Economy)

# Test

if (WIN32)
    add_custom_command(
        TARGET economy_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:economy_tests> $<TARGET_FILE_DIR:economy_tests>
        COMMAND_EXPAND_LISTS
    )
endif ()

gtest_discover_tests(economy_tests)