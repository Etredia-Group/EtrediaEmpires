project(ECS)

# List sources

set(HEADERS "ecs.hpp" "api.hpp" "world.hpp")
set(SOURCES "api.cpp" "world.cpp")

list(TRANSFORM HEADERS PREPEND "include/ee_ecs/")
list(TRANSFORM SOURCES PREPEND "src/")

# Find dependencies
find_package(EnTT CONFIG REQUIRED)

# Link library

add_library(ECS SHARED ${HEADERS} ${SOURCES})
add_library(ECS::ECS ALIAS ECS)

target_link_libraries(ECS PUBLIC Economy::Economy)
target_link_libraries(ECS PRIVATE EnTT::EnTT)

target_include_directories(ECS PUBLIC include/)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ECS PROPERTY CXX_STANDARD 20)
endif()