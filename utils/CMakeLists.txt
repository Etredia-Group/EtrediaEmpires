include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB utils)

# Find dependencies

find_package(Boost REQUIRED COMPONENTS lockfree)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC p7)
target_link_libraries(${LIB} PRIVATE Boost::lockfree)