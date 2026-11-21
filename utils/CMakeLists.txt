include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB utils)

# Options

option(ENABLE_LOGGING "Compile logging into all targets" ON)

# Find dependencies

find_package(Boost REQUIRED COMPONENTS lockfree)
find_package(reflectcpp CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC p7)
target_link_libraries(${LIB} PRIVATE Boost::lockfree)
target_link_libraries(${LIB} PRIVATE reflectcpp::reflectcpp)
target_link_libraries(${LIB} PRIVATE glaze::glaze)
target_link_libraries(${LIB} PRIVATE glm::glm)
target_link_libraries(${LIB} PRIVATE RmlUi::RmlUi)

if (NOT ENABLE_LOGGING)
    target_compile_definitions(${LIB} PUBLIC LOGGER_DISABLED=1)
endif ()