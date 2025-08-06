# List sources

set(SOURCES "test_conf.hpp" "main.cpp"
    "systems/enterprise_routine.cpp" "systems/mining.cpp" "systems/delivering.cpp")

# Find dependencies

find_package(GTest CONFIG REQUIRED)

# Link

include(GoogleTest)

add_executable(economy_tests ${SOURCES})
target_link_libraries(economy_tests PRIVATE GTest::gtest_main)
target_link_libraries(economy_tests PRIVATE Economy::Economy)

# Test

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET economy_tests PROPERTY CXX_STANDARD 20)
endif()

gtest_discover_tests(economy_tests)