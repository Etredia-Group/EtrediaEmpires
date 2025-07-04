# List sources

set(SOURCES "test_conf.hpp" "main.cpp" "world.cpp" "benchmark.cpp")

# Find dependencies

find_package(GTest CONFIG REQUIRED)

# Link

include(GoogleTest)

add_executable(ecs_tests ${SOURCES})
target_link_libraries(ecs_tests PRIVATE GTest::gtest_main)
target_link_libraries(ecs_tests PRIVATE ECS::ECS)

# Test

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ecs_tests PROPERTY CXX_STANDARD 20)
endif()

if (WIN32)
    add_custom_command(
        TARGET ecs_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:ecs_tests> $<TARGET_FILE_DIR:ecs_tests>
        COMMAND_EXPAND_LISTS
    )
endif ()

gtest_discover_tests(ecs_tests)