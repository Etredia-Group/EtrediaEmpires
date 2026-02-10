project(engine-api)

include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

# Find dependencies

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Link library

add_library_target(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE p7)
target_link_libraries(${PROJECT_NAME} PUBLIC server)
target_link_libraries(${PROJECT_NAME} PUBLIC utils)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)