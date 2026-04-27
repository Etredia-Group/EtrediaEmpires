include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB utils)

# Find dependencies

find_package(Boost REQUIRED COMPONENTS lockfree)
find_package(reflectcpp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC p7)
target_link_libraries(${LIB} PRIVATE Boost::lockfree)
target_link_libraries(${LIB} PRIVATE reflectcpp::reflectcpp)
target_link_libraries(${LIB} PRIVATE glm::glm)