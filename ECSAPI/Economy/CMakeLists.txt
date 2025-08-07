project(Economy)

# List sources

set(HEADERS "economy.hpp" "defines.hpp" "enterprise_defaults.hpp" "systems.hpp")
set(HEADERS_COMPONENTS "container.hpp" "enterprise.hpp" "state.hpp" "tags.hpp"
  "miner.hpp" "producer.hpp" "deliverer.hpp")

set(SOURCES "Economy.cpp")
set(SOURCES_SYSTEMS "enterprise_routine.cpp" "mining.cpp" "delivering.cpp")

list(TRANSFORM HEADERS PREPEND "include/ee_economy/")
list(TRANSFORM HEADERS_COMPONENTS PREPEND "include/ee_economy/components/")

list(TRANSFORM SOURCES PREPEND "src/")
list(TRANSFORM SOURCES_SYSTEMS PREPEND "src/systems/")

# Find dependencies

find_package(EnTT CONFIG REQUIRED)

# Link library

add_library(${PROJECT_NAME} ${HEADERS} ${HEADERS_COMPONENTS} ${SOURCES} ${SOURCES_SYSTEMS})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE EnTT::EnTT)

target_include_directories(${PROJECT_NAME} PUBLIC include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()