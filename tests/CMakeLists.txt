include(${CMAKE_SOURCE_DIR}/cmake/executable.cmake)

set(EXE tests)

# Find dependencies

find_package(GTest CONFIG REQUIRED)

# Link library

include(GoogleTest)

add_executable_target(${EXE})
set_target_properties(${EXE} PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_link_libraries(${EXE} PRIVATE GTest::gtest_main)
target_link_libraries(${EXE} PUBLIC utils)
target_link_libraries(${EXE} PUBLIC engine-api)
target_link_libraries(${EXE} PUBLIC entt-module)
target_link_libraries(${EXE} PUBLIC ecs-context)
target_link_libraries(${EXE} PUBLIC economy)
target_link_libraries(${EXE} PUBLIC ecs-world)

# Test

if (WIN32)
    add_custom_command(
            TARGET ${EXE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${EXE}> $<TARGET_FILE_DIR:${EXE}>
            COMMAND_EXPAND_LISTS
    )
endif ()

gtest_discover_tests(${EXE})