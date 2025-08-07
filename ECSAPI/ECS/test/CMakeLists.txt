project(ecs_tests)

# List sources

set(SOURCES "test_conf.hpp" "main.cpp" "world.cpp" "benchmark.cpp")

# Find dependencies

find_package(GTest CONFIG REQUIRED)

# Link

include(GoogleTest)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main)
target_link_libraries(${PROJECT_NAME} PRIVATE ECS::ECS)

# Test

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

gtest_discover_tests(${PROJECT_NAME})