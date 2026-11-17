include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB entt-module)

# Find dependencies

find_package(EnTT CONFIG REQUIRED)

# Link library

add_compile_definitions(ENTT_USE_ATOMIC)
add_library_target(${LIB})

target_link_libraries(${LIB} PRIVATE EnTT::EnTT)