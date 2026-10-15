include(${CMAKE_SOURCE_DIR}/cmake/executable.cmake)

set(EXE sdk)

# Find dependencies

find_package(cxxopts CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

# Link library

add_executable_target(${EXE})

target_link_libraries(${EXE} PUBLIC utils)
target_link_libraries(${EXE} PUBLIC engine-api)

target_link_libraries(${EXE} PUBLIC p7)
target_link_libraries(${EXE} PRIVATE cxxopts::cxxopts)
target_link_libraries(${EXE} PRIVATE reflectcpp::reflectcpp)
target_link_libraries(${EXE} PRIVATE glm::glm)
target_link_libraries(${EXE} PRIVATE xxHash::xxhash)