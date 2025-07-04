project(Economy)

# List sources

set(HEADERS "economy.hpp" "defines.hpp" "enterprise_defaults.hpp" "systems.hpp")
set(HEADERS_COMPONENTS "container.hpp" "enterprise.hpp" "miner.hpp" "producer.hpp" "state.hpp" "tags.hpp")

set(SOURCES "Economy.cpp")
set(SOURCES_SYSTEMS "mining.cpp")

list(TRANSFORM HEADERS PREPEND "include/ee_economy/")
list(TRANSFORM HEADERS_COMPONENTS PREPEND "include/ee_economy/components/")

list(TRANSFORM SOURCES PREPEND "src/")
list(TRANSFORM SOURCES_SYSTEMS PREPEND "src/systems/")

# Find dependencies

find_package(EnTT CONFIG REQUIRED)

# Link library

add_library(Economy SHARED ${HEADERS} ${HEADERS_COMPONENTS} ${SOURCES} ${SOURCES_SYSTEMS})
add_library(Economy::Economy ALIAS Economy)

target_link_libraries(Economy PRIVATE EnTT::EnTT)

target_include_directories(Economy PUBLIC include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Economy PROPERTY CXX_STANDARD 20)
endif()