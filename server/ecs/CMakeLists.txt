project(ECS)

# List sources

set(HEADERS "ecs.hpp" "api.hpp" "world.hpp")
set(SOURCES "api.cpp" "world.cpp")

list(TRANSFORM HEADERS PREPEND "include/ee_ecs/")
list(TRANSFORM SOURCES PREPEND "src/")

# Find dependencies
find_package(EnTT CONFIG REQUIRED)

# Link library

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC Economy::Economy)
target_link_libraries(${PROJECT_NAME} PRIVATE EnTT::EnTT)

target_include_directories(${PROJECT_NAME} PUBLIC include/)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()