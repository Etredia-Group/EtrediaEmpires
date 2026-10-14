include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB engine-api)

# Find dependencies

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC utils)
target_link_libraries(${LIB} PUBLIC economy)
target_link_libraries(${LIB} PRIVATE glfw)
target_link_libraries(${LIB} PRIVATE glm::glm)