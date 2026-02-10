include(${CMAKE_SOURCE_DIR}/cmake/library.cmake)

set(LIB magnum-renderer)

# Find dependencies

if (WIN32)
  set(MAGNUM_CONTEXT WglContext)
elseif(UNIX)
  set(MAGNUM_CONTEXT GlxContext)
else()
  message(FATAL_ERROR "No supported context for system")
endif()

set(MAGNUM_WITH_GLFWAPPLICATION ON)
find_package(Magnum REQUIRED
    GL
    GlfwApplication
    Shaders
)

# Link library

add_library_target(${LIB})

target_link_libraries(${LIB} PUBLIC engine-api)
target_link_libraries(${LIB} PUBLIC utils)

target_link_libraries(${LIB} PUBLIC p7)
target_link_libraries(${LIB} PRIVATE
    Magnum::GL
    Magnum::GlfwApplication
    Magnum::Shaders
    Magnum::Magnum)