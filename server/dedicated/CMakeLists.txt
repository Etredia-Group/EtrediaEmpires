include(${CMAKE_SOURCE_DIR}/cmake/executable.cmake)

set(EXE server_dedicated)

# Find dependencies

# Link library

add_executable_target(${EXE})

target_link_libraries(${EXE} PRIVATE utils)
target_link_libraries(${EXE} PRIVATE server)