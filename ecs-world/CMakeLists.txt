include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB ecs-world)

# Find dependencies

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC utils)
target_link_libraries(${LIB} PUBLIC entt-module)
target_link_libraries(${LIB} PUBLIC ecs-context)
target_link_libraries(${LIB} PUBLIC economy)