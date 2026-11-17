include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB ecs-context)

# Find dependencies

find_package(reflectcpp CONFIG REQUIRED)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC utils)
target_link_libraries(${LIB} PUBLIC entt-module)
target_link_libraries(${LIB} PRIVATE reflectcpp::reflectcpp)
