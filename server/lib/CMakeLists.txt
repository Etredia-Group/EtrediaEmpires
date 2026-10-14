include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB server)

# Find dependencies

find_package(GameNetworkingSockets CONFIG REQUIRED)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PRIVATE utils)
target_link_libraries(${LIB} PRIVATE GameNetworkingSockets::shared)