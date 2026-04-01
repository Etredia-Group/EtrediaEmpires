include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB vulkan-renderer)

# Find dependencies

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)

# Link library

add_library_target(${LIB}
    MODULES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/vulkan/vulkan.cppm
    MODULE_BASEDIRS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/
)

target_compile_definitions(${LIB}
    PUBLIC
    VULKAN_HPP_NO_CONSTRUCTORS
    VULKAN_HPP_ENABLE_STD_MODULE
    VULKAN_HPP_STD_MODULE=std
)

target_link_libraries(${LIB} PUBLIC engine-api)
target_link_libraries(${LIB} PUBLIC utils)

target_link_libraries(${LIB} PUBLIC p7)
target_link_libraries(${LIB} PRIVATE glm::glm)
target_link_libraries(${LIB} PRIVATE Vulkan::Vulkan)